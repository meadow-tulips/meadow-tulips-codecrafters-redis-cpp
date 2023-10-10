#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <cstdint>
#include <iomanip>
#include "Parser/parser.h"

unsigned int ToHex(char x)
{
  return (0xff & (unsigned int)x);
}
bool isMagicString(char *temp)
{
  if (strlen(temp) == 5)
  {
    const char *s = "REDIS";
    for (int i = 0; i < strlen(temp); i++)
    {
      if (ToHex(temp[i]) != ToHex(s[i]))
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

char *getNextCharacterAndGatherBytes(std::fstream &fs)
{
  char nextCharacter;
  fs.read(&nextCharacter, 1);
  char *word = new char[((int)nextCharacter) + 1]();
  fs.read(word, nextCharacter);
  return word;
}

void parseKeyValuePairs(std::fstream &fs, char &characterInMemory, std::unordered_set<DataEntity, DataEntityHashFunction> &entitiesCollection)
{
  long unsigned int expiryMs{0};
  unsigned int expirySec{0};
  if (ToHex(characterInMemory) == 0xFD)
  {
    // Read 4 bytes
    fs.read((char *)&expirySec, 4);
    fs.read(&characterInMemory, 1); // value type
  }
  else if (ToHex(characterInMemory) == 0xFC)
  {
    // Read next 8 bytes
    std::string hexString;
    fs.read((char *)&expiryMs, 8);
    fs.read(&characterInMemory, 1); // value type
  }
  std::string key = getNextCharacterAndGatherBytes(fs);
  std::string value = getNextCharacterAndGatherBytes(fs);

  if (expiryMs == 0 && expirySec == 0)
  {
    entitiesCollection.insert(DataEntity(key, value, true));
    return;
  }

  long long expiry{0};
  if (expirySec > 0)
    expiry = std::chrono::milliseconds(expirySec * 1000).count();
  else
    expiry = std::chrono::milliseconds(expiryMs).count();

  std::chrono::system_clock::duration now = std::chrono::system_clock::now().time_since_epoch();
  auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

  if (expiry > currentTime)
  {
    entitiesCollection.insert(DataEntity(key, value, true, std::to_string(expiry - currentTime)));
  }
}

void parseRDB(std::fstream &fs, std::streampos fileSize, std::unordered_set<DataEntity, DataEntityHashFunction> &entitiesCollection)
{
  // Begin reading  MAGIC word first
  char *buffer = new char[6]();
  fs.read(buffer, 5);
  int totalHashSize = 0;

  if (!isMagicString(buffer))
  {
    std::cout << "Not a redis rdb file" << std::endl;
    return;
  }
  fs.seekg(4, std::ios_base::cur);
  char characterInMemory;
  fs.read(&characterInMemory, 1);
  while (fs.tellg() < fileSize)
  {
    if (ToHex(characterInMemory) == 0xFF)
      break;
    if (ToHex(characterInMemory) == 0xFE)
    {
      fs.read(&characterInMemory, 1); // DB Selector
      fs.read(&characterInMemory, 1); // 0xFB
      fs.read(&characterInMemory, 1); // hash size;
      totalHashSize += (int)(characterInMemory);
      fs.read(&characterInMemory, 1); // expiry hash size;
    }
    else
    {
      if (totalHashSize > 0)
      {
        parseKeyValuePairs(fs, characterInMemory, entitiesCollection);
        totalHashSize--;
      }
    }
    fs.read(&characterInMemory, 1);
  }
}

int main(int argc, char **argv)
{
  // You can use print statements as follows for debugging, they'll be visible when running tests.

  std::unordered_set<DataEntity, DataEntityHashFunction> entitiesCollection;
  std::string fullFilePath = "";
  entitiesCollection.clear();

  std::cout << " Binding IP Address to a port" << std::endl;
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i - 1];
    if (arg == "--dir" || arg == "--dbfilename")
    {
      std::string optionType = arg.substr(2, arg.length());
      std::string value = argv[i];
      entitiesCollection.insert(DataEntity(optionType, value));

      fullFilePath += (fullFilePath == "" ? value : "/" + value);
    }
  }

  std::fstream fs{fullFilePath, std::ios_base::binary | std::ios_base::in};
  std::streampos fileSize;

  if (fs.is_open())
  {
    std::cout << "File is opened" << std::endl;
    fs.seekg(0, std::ios_base::end);

    fileSize = fs.tellg();
    fs.seekg(0, std::ios::beg);
    parseRDB(fs, fileSize, entitiesCollection);
  }
  else
  {
    std::cout << "Failure! File open Failed." << std::endl;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  fcntl(server_fd, F_SETFL, O_NONBLOCK);

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  int bind_result = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  if (bind_result != 0)
  {
    std::cout << "Socket Address Bind Failed" << std::endl;
  }

  int connection_backlog = 5;

  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "Listen Failed" << std::endl;
  }

  int connection_count = 1;
  struct pollfd *pollFdsCollection = new struct pollfd[connection_count]();
  struct pollfd serverpfd[1];
  pollFdsCollection[0].fd = server_fd;
  pollFdsCollection[0].events = POLLIN;

  while (true)
  {
    int num_events = poll(pollFdsCollection, connection_count, -1);
    if (num_events > 0)
    {

      for (int i = 0; i < connection_count; i++)
      {
        int pollin_happened = pollFdsCollection[i].revents & POLLIN;
        if (pollin_happened)
        {
          if (i == 0)
          {
            // Thats our server pollin, ready to accept connections
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof(client_address);

            int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
            fcntl(client_fd, F_SETFL, O_NONBLOCK);

            connection_count++;

            struct pollfd *pollFdsCollectionCopy = pollFdsCollection;
            pollFdsCollection = new struct pollfd[connection_count]();
            memcpy(pollFdsCollection, pollFdsCollectionCopy, (connection_count - 1) * sizeof(*pollFdsCollectionCopy));
            delete[] pollFdsCollectionCopy;
            pollFdsCollection[connection_count - 1].fd = client_fd;
            pollFdsCollection[connection_count - 1].events = POLLIN;
            std::cout << "Client connected" << std::endl;
          }
          else
          {
            // Thats our clients
            char receivedBuffer[100];
            int client_fd = pollFdsCollection[i].fd;
            ssize_t receivedBytes = recv(client_fd, receivedBuffer, sizeof(receivedBuffer), 0);
            if (receivedBytes > 0)
            {
              Parser _parser(receivedBuffer);
              std::string response = _parser.recursivelyParseTokens(0, "", "", entitiesCollection);

              if (response != "")
                send(client_fd, &response[0], response.length(), 0);
            }
          }
        }
      }
    }
  }

  close(server_fd);

  return 0;
}
