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
#include<poll.h>


void constructEventLoop(int fd_server) {
  fd_set read_fds;
  FD_ZERO(&read_fds);  

  // Add a descriptor to an fd_set
  FD_SET(fd_server, &read_fds);  

  // select(2, &read_fds);


  FD_ISSET(fd_server, &read_fds); 
}

int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.

  std::cout<<" Binding IP Address to a port"<<std::endl;
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

  fcntl(server_fd, F_SETFL, O_NONBLOCK);


  
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

  struct pollfd serverpfd[1];
  serverpfd[0].fd = server_fd;
  serverpfd[0].events = POLLIN;

  struct pollfd* connections = new struct pollfd[(2 * connection_backlog) + 1]; 
  int connection_count = -1;

  while(true) {
      int num_events = poll(serverpfd, 1, 1000);

      if(num_events == 0) {
        std::cout<<"Poll timed out"<<std::endl;
      } else {
        int pollin_happened = serverpfd[0].revents & POLLIN;
        if(pollin_happened) {
        struct sockaddr_in client_address;
        int client_addr_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr* )&client_address, (socklen_t *) &client_addr_len);
        fcntl(client_fd, F_SETFL, O_NONBLOCK);

        connection_count++;
        connections[connection_count].fd = client_fd;
        connections[connection_count].events = POLLIN;
            
      
        std::cout<<"Client connected"<<std::endl;

        }
      }



      int connection_events = poll(connections, connection_backlog, -1);

      if(connection_events == 0) {
        std::cout<<"Poll timed out"<<std::endl;
      } else {
        for(int i=0;i<(connection_count + 1);i++) {
          int pollin_happened = connections[i].revents & POLLIN;
          if(pollin_happened) {
            const char* msg = "+PONG\r\n";
            char receivedBuffer[100];
            int client_fd = connections[i].fd;
            ssize_t receivedBytes = recv(client_fd, receivedBuffer, sizeof(receivedBuffer), MSG_EOR);
            if(receivedBytes > 0) {
              strstr(receivedBuffer, "PING\r\n");
              std::cout<<"Messaged received"<<receivedBuffer<<std::endl;
              std::cout<<"Message Ended"<<std::endl;
              const char* stringToSearch = "ping";
              char* result = strstr(receivedBuffer, stringToSearch);
              while(result != NULL) {
                send(client_fd, msg, strlen(msg), MSG_EOR);
                strcpy(result, "");
                result = strstr(receivedBuffer, stringToSearch);
              }
            }
          }
        }
      }
  }

  close(server_fd);


  return 0;
}
