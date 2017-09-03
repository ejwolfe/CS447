//Include files
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Defines
#define PORT 9080
#define BUF_SIZE 4096
// HTTP response messages
#define OK_IMAGE    "HTTP/1.0 200 OK\nContent-Type:image/gif\n\n"
#define OK_TEXT     "HTTP/1.0 200 OK\nContent-Type:text/html\n\n"
#define NOTOK_404   "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n"
#define MESS_404    "<html><body><h1>FILE NOT FOUND</h1></body></html>"

void *handle_get(void *in_arg);       // Thread function to handle GET

using namespace std;

int main(int argc, char const *argv[]) {
  unsigned int        proxySocket, clientSocket;    //Proxy and Client descriptor
  struct sockaddr_in  proxyAddress, clientAddress;  //Proxy and Client address
  struct in_addr      clientIPAddress;              //Client IP address
  int                 addressLength;                //Internet address length
  pthread_t           thread1;

  // Create a socket, fill-in address information, and then bind it
  proxySocket = socket(AF_INET, SOCK_STREAM, 0);
  proxyAddress.sin_family = AF_INET;
  proxyAddress.sin_port = htons(PORT);
  proxyAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(proxySocket, (struct sockaddr *)&proxyAddress, sizeof(proxyAddress));

  // Main loop to listen, accept, and then spin-off a thread to handle the GET
  while (1)
  {
     // Listen for connections and then accept
     listen(proxySocket, 100);
     addressLength = sizeof(clientAddress);
     clientSocket = accept(proxySocket, (struct sockaddr *)&clientAddress, (socklen_t*)&addressLength);
     if (clientSocket == 0)
     {
        printf("ERROR - Unable to create socket \n");
        exit(1);
     }

     /* Display the signature of the new client ----- */
     memcpy(&clientIPAddress, &clientAddress.sin_addr.s_addr, 4);
     printf("\n*** Profile for a connecting client ***\n");
     printf("IP address: %s\n", inet_ntoa(clientIPAddress));
     printf("Client-side Port: %d\n", ntohs(clientAddress.sin_port));
     printf("\n");

     // Spin-off a thread to handle this request (pass only clientSocket)
     if (pthread_create(&thread1, 0, handle_get, (void *) &clientSocket) < 0)
     {
        printf("ERROR - Unable to create thread \n");
        exit(1);
     }
     /* ------------------------------------------------------------------- */
  }
  pthread_join(thread1, NULL);
  close(proxySocket);
  return 0;
}

//===========================================================================
//=  This is is the thread function to handle the GET                       =
//=   - It is assumed that the request is a GET                             =
//===========================================================================
void *handle_get(void *in_arg)
{
   unsigned int   clientSocket;             // Client socket descriptor
   char           in_buf[BUF_SIZE];     // Input buffer for GET resquest
   char           out_buf[BUF_SIZE];    // Output buffer for HTML response
   char           *file_name;           // File name
   unsigned int   fh;                   // File handle
   unsigned int   buf_len;              // Buffer length for file reads
   unsigned int   retcode;              // Return code

                                        // Set clientSocket to in_arg
   clientSocket = *((unsigned int *) in_arg);

   // Receive the GET request from the Web browser
   retcode = recv(clientSocket, in_buf, BUF_SIZE, 0);

   // Handle the GET if there is one (see note #3 in the header)
   if (retcode != -1)
   {
      // Parse out the filename from the GET request
      strtok(in_buf, " ");
      file_name = strtok(NULL, " ");

      // Open the requested file
      //  - Start at 2nd char to get rid of leading "\"
      fh = open(&file_name[1], 0, S_IREAD | S_IWRITE);

      // Generate and send the response (404 if could not open the file)
      if (fh == -1)
      {
         printf("File %s not found - sending an HTTP 404 \n", &file_name[1]);
         strcpy(out_buf, NOTOK_404);
         send(clientSocket, out_buf, strlen(out_buf), 0);
         strcpy(out_buf, MESS_404);
         send(clientSocket, out_buf, strlen(out_buf), 0);
      }

      else
      {
         printf("File %s is being sent \n", &file_name[1]);
         if (strstr(file_name, ".gif") != NULL)
            strcpy(out_buf, OK_IMAGE);
         else
            strcpy(out_buf, OK_TEXT);

         send(clientSocket, out_buf, strlen(out_buf), 0);
         while (!fh)
         {
            buf_len = read(fh, out_buf, BUF_SIZE);
            send(clientSocket, out_buf, buf_len, 0);
         }
         close(fh);
      }
   }

   // Close the client socket and end the thread
   close(clientSocket);
   pthread_exit(0);
}
