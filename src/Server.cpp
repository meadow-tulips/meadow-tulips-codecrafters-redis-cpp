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
  std::cout << "Logs from your program will appear here!\n";
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

  std::string msg = "+PONG\r\n";

  const void* msg_buffer = msg.c_str();

  ssize_t sendBytes = send(client_fd, msg_buffer, sizeof(msg_buffer) - 1, MSG_EOR);

  if(sendBytes == -1) {
    std::cerr<<"Failed to send message"<<std::endl;
  }


  close(server_fd);


  return 0;
}
