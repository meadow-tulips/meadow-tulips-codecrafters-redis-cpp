#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<vector>

int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout<<" Binding IP Address to a port"<<std::endl;
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  int bind_result = bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr));

  if(bind_result != 0) {
    std::cout<<"Socket Address Bind Failed"<<std::endl;
  }


  int connection_backlog = 5;

  if(listen(server_fd, connection_backlog) != 0) {
    std::cerr<<"Listen Failed"<<std::endl;
  }

  struct sockaddr_in client_address;
  int client_addr_len = sizeof(client_address);


  int client_fd = accept(server_fd, (struct sockaddr* )&client_address, (socklen_t *) &client_addr_len);

  std::cout<<"Client connected"<<std::endl;

  const char* msg = "+PONG\r\n";
  char receivedBuffer[100];

  ssize_t receivedBytes = recv(client_fd, receivedBuffer, sizeof(receivedBuffer), MSG_PEEK);

  while(receivedBytes > 0) {
      receivedBytes = recv(client_fd, receivedBuffer, sizeof(receivedBuffer), 0);
      ssize_t sendBytes = send(client_fd, msg, strlen(msg), MSG_EOR);
            if(sendBytes == -1) {
          std::cerr<<"Failed to send message"<<std::endl;
        } else {
          std::cout<<"Successfully sent "<<sendBytes<<" bytes to the client"<<std::endl;
        }
  }

  

  



  close(server_fd);


  return 0;
}
