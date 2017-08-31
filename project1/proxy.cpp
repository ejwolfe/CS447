#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#define PORT 9080

using namespace std;

int main(int argc, char const *argv[]){
  struct sockaddr_in my_addr, client_addr;
  unsigned int socket_id, child_sock;
  int bindStatus, listenStatus;

  //Check to see if the socket can be created
  if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0){
    cerr << "Socket was not initialized" << endl;
    exit(1);
  }
  //Check to see if the AF, IP, and Port can be bound to the socket
  if ((bindStatus = bind(socket_id, (struct sockaddr *)&my_addr, sizeof(my_addr))) < 0){
    cerr << "Socket could not be bound" << endl;
    exit(1);
  }

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((listenStatus = listen(socket_id, 3)) < 0){
    cerr << "Error with listen function" << endl;
    exit(1);
  }

  return 0;
}
