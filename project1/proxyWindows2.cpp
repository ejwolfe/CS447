// Eric Wolfe
// CS 447
// Project 1
// Last Edited 11:07 PM 10/2/20117

#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for exit()
#include <string>           // Needed for strcpy() and strlen()
#include <fcntl.h>          // Needed for file i/o constants
#include <sys/stat.h>       // Needed for file i/o constants
#include <stddef.h>         // Needed for _threadid
#include <process.h>        // Needed for _beginthread() and _endthread()
#include <io.h>             // Needed for open(), close(), and eof()
#include <windows.h>        // Needed for all Winsock stuff
#include <vector>           //Needed for client and server sockets

using namespace std;

// Defines
#define PROXYPORT 9080
#define BUFFERSIZE 1024
#define SERVERPORT 1080
#define SERVERIP "127.0.0.1"

// HTTP response messages
#define OK_IMAGE    "HTTP/1.0 200 OK\nContent-Type:image/gif\n\n"
#define OK_TEXT     "HTTP/1.0 200 OK\nContent-Type:text/html\n\n"
#define NOTOK_404   "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n"
#define MESS_404    "<html><body><h1>FILE NOT FOUND</h1></body></html>"
#define NOTOK_401	"HTTP/1.0 401 Unauthorized Access\nContent-Type:text/html\n\n"
#define MESS_401	"<html><body><h1>Unauthorized Access</h1></body></html>"
// Hazardous Phrases
char hazardous_contents_CS_01[256] = ""; /* C->S hazardous contents 1 */
char hazardous_contents_CS_02[256] = ""; /* C->S hazardous contents 2 */
char hazardous_contents_SC_01[256] = ""; /* S->C hazardous contents 1 */
char hazardous_contents_SC_02[256] = ""; /* S->C hazardous contents 2 */
// Prototypes
char *checkoutForHazardous(unsigned int clientSocketPosition); //Thread for sending information from the client to the server
void serverToClient(void *in_args); //Thread for sending information from the server to the client
// Return code
unsigned int checkReturnCode;

int main(int argc, char const *argv[]) {
	printf("This is the proxy\n");
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
	unsigned int        proxySocket, serverSocketConnection, clientSocket, serverSocket;
	struct sockaddr_in  proxyAddress, clientAddress, serverAddress;
	struct in_addr      clientIPAddress, serverIPAddress;
	int                 addressLength, clientSocketPosition = 0, serverSocketPosition = 0;
	char                buffer[BUFFERSIZE];

	// Create a socket, fill-in address information, bind it, and then listen
	proxySocket = socket(AF_INET, SOCK_STREAM, 0);
	proxyAddress.sin_family = AF_INET;
	proxyAddress.sin_port = htons(PROXYPORT);
	proxyAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(proxySocket, (struct sockaddr *)&proxyAddress, sizeof(proxyAddress));
	listen(proxySocket, 100);

	// Main loop to accept, create new socket for server, connect to server, and start both threads
	while (1) {
		checkReturnCode = 0;
		//Accept connection from client
		addressLength = sizeof(clientAddress);
		if ((clientSocket= accept(proxySocket, (struct sockaddr *)&clientAddress, &addressLength)) == 0) {
			printf("ERROR - Unable to create socket \n");
			exit(1);
		}
		strcpy(buffer, checkoutForHazardous(clientSocket));

		if (strcmp(buffer, "") != 0) {
			/* Display the signature of the new client ----- */
			memcpy(&clientIPAddress, &clientAddress.sin_addr.s_addr, 4);
			printf("\n*** Profile for a connecting client ***\n");
			printf("IP address: %s\n", inet_ntoa(clientIPAddress));
			printf("Client-side Port: %d\n", ntohs(clientAddress.sin_port));
			printf("\n");

			// Create the server socket, fill-in address information, and then connect
			serverSocket = socket(AF_INET, SOCK_STREAM, 0);
			serverAddress.sin_family = AF_INET;
			serverAddress.sin_port = htons(SERVERPORT);
			serverAddress.sin_addr.s_addr = inet_addr(SERVERIP);
			connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
			send(serverSocket, buffer, checkReturnCode, 0);
			unsigned int threadArgs[2] = { clientSocket, serverSocket };
			// Setting up arguments for the threads (easier to pass socket information) and then start threads
			if (_beginthread(serverToClient, 4096, (void *)threadArgs) < 0) {
				printf("ERROR - Unable to create client to server thread \n");
				exit(1);
			}
		}
	}
	closesocket(proxySocket);
	WSACleanup();
	return 0;
}

char *checkoutForHazardous(unsigned int clientSocket) {
	char inBuffer[BUFFERSIZE];

	checkReturnCode = recv(clientSocket, inBuffer, strlen(inBuffer), 0);

	if (checkReturnCode <= 0) {
		strcpy(inBuffer, NOTOK_404);
		send(clientSocket, inBuffer, strlen(inBuffer), 0);
		strcpy(inBuffer, MESS_404);
		send(clientSocket, inBuffer, strlen(inBuffer), 0);
	}
	else {
		if (strcmp(hazardous_contents_CS_01, "") != 0) {
			if (strstr(inBuffer, hazardous_contents_CS_01)) {
				strcpy(inBuffer, NOTOK_401);
				send(clientSocket, inBuffer, strlen(inBuffer), 0);
				strcpy(inBuffer, MESS_401);
				send(clientSocket, inBuffer, strlen(inBuffer), 0);
				return "";
			}
		}
		if (strcmp(hazardous_contents_CS_02, "") != 0) {
			if (strstr(inBuffer, hazardous_contents_CS_02)) {
				strcpy(inBuffer, NOTOK_401);
				send(clientSocket, inBuffer, strlen(inBuffer), 0);
				strcpy(inBuffer, MESS_401);
				send(clientSocket, inBuffer, strlen(inBuffer), 0);
				return "";
			}
		}
		return inBuffer;
	}
	return "";
}

void serverToClient(void *in_args) {
	unsigned int  fh, bufferLength, returnCode;
	unsigned int	clientSocket, serverSocket;
	clientSocket = ((unsigned int *)in_args)[0];
	serverSocket = ((unsigned int *)in_args)[1];

	while (1) {
		char inBuffer[BUFFERSIZE];
		char outBuffer[BUFFERSIZE];
		returnCode = recv(serverSocket, inBuffer, BUFFERSIZE, 0);
		if (returnCode == -1) {
			printf("Error occured with server connection");
			break;
		}
		else if (returnCode == 0) {
			printf("Disconnected from server");
			break;
		}
		else {
			if (strcmp(hazardous_contents_SC_01, "") != 0 || strcmp(hazardous_contents_SC_02, "") != 0) {
				if (strstr(inBuffer, hazardous_contents_SC_01) || strstr(inBuffer, hazardous_contents_SC_02)) {
					strcpy(outBuffer, NOTOK_401);
					send(clientSocket, outBuffer, strlen(outBuffer), 0);
					strcpy(outBuffer, MESS_401);
					send(clientSocket, outBuffer, strlen(outBuffer), 0);
					break;
				}
				else {
					send(clientSocket, inBuffer, returnCode, 0);
				}
			}
			else {
				send(clientSocket, inBuffer, returnCode, 0);
			}
		}
	}
	closesocket(serverSocket);
	closesocket(clientSocket);
	_endthread();
}
