#include <iostream>
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
#include <vector>
#include <unordered_map>
#include <chrono>

void parseInstructions(char *stringBuffer, ssize_t size, int client_fd, std::unordered_map<std::string, std::pair<std::string, std::chrono::system_clock::duration>> &values, const std::chrono::system_clock::duration &currentTime)
{

  int lookOutCharacters = 0;
  bool isDollarFound = false;
  std::string temporary = "";
  std::string instruction = "";
  std::vector<std::string> instructionArguments;
  for (int i = 0; i < size; i++)
  {
    if (stringBuffer[i] == '$')
    {
      isDollarFound = true;
      continue;
    }

    if (isDollarFound)
    {
      if (stringBuffer[i] == '\r' || stringBuffer[i] == '\n')
      {
        isDollarFound = false;
        // std::cout<<temporary<<" "<<i<<"-i"<<std::endl;
        lookOutCharacters = stoi(temporary);
        temporary = "";
      }
      else
      {
        temporary += stringBuffer[i];
      }
      continue;
    }

    if (stringBuffer[i] == '\r' || stringBuffer[i] == '\n')
      continue;

    if (lookOutCharacters > 0)
    {
      temporary += stringBuffer[i];
      lookOutCharacters--;
    }

    if (lookOutCharacters == 0 && instruction == "" && temporary != "")
    {
      instruction = temporary;
      temporary = "";
    }

    if (lookOutCharacters == 0 && instruction != "" && temporary != "")
    {
      instructionArguments.push_back(temporary);
      temporary = "";
    }
  }

  if (instruction == "ping")
  {
    const char *msg = "+PONG\r\n";
    send(client_fd, msg, strlen(msg), 0);
  }

  if (instruction == "echo" && instructionArguments.size() > 0)
  {
    std::string argument = ":" + instructionArguments[0] + "\r\n";
    send(client_fd, &argument[0], argument.length(), 0);
  }

  if (instruction == "set" && instructionArguments.size() > 1)
  {
    std::chrono::milliseconds ms(3600000);

    if (instructionArguments.size() > 3 && instructionArguments[2] == "px")
      ms = std::chrono::milliseconds(stoi(instructionArguments[3]));
          
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    auto expiry = now + ms;

    std::pair<std::string, std::chrono::system_clock::duration> valueWithExpiry = std::make_pair(instructionArguments[1], expiry);

    values[instructionArguments[0]] = valueWithExpiry;

    const char *msg = ":OK\r\n";
    send(client_fd, msg, strlen(msg), 0);
  }

  if (instruction == "get" && instructionArguments.size() > 0)
  {
    if (values.find(instructionArguments[0]) != values.end())
    {
      auto tup = values.at(instructionArguments[0]);

      std::string instructionToSend = std::get<0>(tup);
      std::chrono::system_clock::duration expiryTime = std::get<1>(tup);

      const auto currentTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
      const auto expiredTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(expiryTime).count();
      std::cout << currentTimeMS << " currentTime " << expiredTimeMS << " ExpiredTime" << std::endl;

      if (currentTimeMS > expiredTimeMS)
      {
        std::string val = ":_\r\n";
        send(client_fd, &val[0], val.length(), 0);
      }
      else
      {
        std::string val = ":" + instructionToSend + "\r\n";
        send(client_fd, &val[0], val.length(), 0);
      }
    }
  }
}

int main(int argc, char **argv)
{
  // You can use print statements as follows for debugging, they'll be visible when running tests.

  std::cout << " Binding IP Address to a port" << std::endl;
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

  std::unordered_map<std::string, std::pair<std::string, std::chrono::system_clock::duration>> values;
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

  struct pollfd serverpfd[1];
  serverpfd[0].fd = server_fd;
  serverpfd[0].events = POLLIN;

  struct pollfd *connections = new struct pollfd[(2 * connection_backlog) + 1];
  int connection_count = -1;

  while (true)
  {
    int num_events = poll(serverpfd, 1, 1000);

    if (num_events == 0)
    {
      std::cout << "Poll timed out" << std::endl;
    }
    else
    {
      int pollin_happened = serverpfd[0].revents & POLLIN;
      if (pollin_happened)
      {
        struct sockaddr_in client_address;
        int client_addr_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_addr_len);
        fcntl(client_fd, F_SETFL, O_NONBLOCK);

        connection_count++;
        connections[connection_count].fd = client_fd;
        connections[connection_count].events = POLLIN;

        std::cout << "Client connected" << std::endl;
      }
    }

    int connection_events = poll(connections, connection_backlog, -1);

    if (connection_events == 0)
    {
      std::cout << "Poll timed out" << std::endl;
    }
    else
    {
      for (int i = 0; i < (connection_count + 1); i++)
      {
        int pollin_happened = connections[i].revents & POLLIN;
        if (pollin_happened)
        {
          const auto now = std::chrono::system_clock::now().time_since_epoch();
          const char *msg = "+PONG\r\n";
          char receivedBuffer[100];
          int client_fd = connections[i].fd;
          ssize_t receivedBytes = recv(client_fd, receivedBuffer, sizeof(receivedBuffer), MSG_EOR);
          if (receivedBytes > 0)
          {
            parseInstructions(receivedBuffer, receivedBytes, client_fd, values, now);
          }
        }
      }
    }
  }

  close(server_fd);

  return 0;
}
