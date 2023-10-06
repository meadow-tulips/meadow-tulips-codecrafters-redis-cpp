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
#include <chrono>
#include "Parser/parser.h"

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

  std::unordered_set<DataEntity, DataEntityHashFunction> entitiesCollection;
  entitiesCollection.clear();

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
