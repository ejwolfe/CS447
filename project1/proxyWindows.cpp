#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for exit()
#include <string>         // Needed for strcpy() and strlen()
#include <fcntl.h>          // Needed for file i/o constants
#include <sys/stat.h>       // Needed for file i/o constants
#include <stddef.h>         // Needed for _threadid
#include <process.h>        // Needed for _beginthread() and _endthread()
#include <io.h>             // Needed for open(), close(), and eof()
#include <windows.h>        // Needed for all Winsock stuff
#include <vector>

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
void clientToServer(void *in_args); //Thread for sending information from the client to the server
void serverToClient(void *in_args); //Thread for sending information from the server to the client
//unsigned int clientSocket, serverSocket;
vector<unsigned int> clientSocket;
vector<unsigned int> serverSocket;

int main(int argc, char const *argv[]) {
	printf("This is the proxy\n");
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
	unsigned int        proxySocket, serverSocketConnection, clientSocketTemp, serverSocketTemp;
	struct sockaddr_in  proxyAddress, clientAddress, serverAddress;
	struct in_addr      clientIPAddress, serverIPAddress;
	int                 addressLength, clientSocketPosition = 0, serverSocketPosition = 0;;

	// Create a socket, fill-in address information, bind it, and then listen
	proxySocket = socket(AF_INET, SOCK_STREAM, 0);
	proxyAddress.sin_family = AF_INET;
	proxyAddress.sin_port = htons(PROXYPORT);
	proxyAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(proxySocket, (struct sockaddr *)&proxyAddress, sizeof(proxyAddress));
	listen(proxySocket, 100);

	// Main loop to accept, create new socket for server, connect to server, and start both threads

	while (1) {
		addressLength = sizeof(clientAddress);
		if ((clientSocketTemp = accept(proxySocket, (struct sockaddr *)&clientAddress, &addressLength)) == 0) {
			printf("ERROR - Unable to create socket \n");
			exit(1);
		}
		clientSocket.push_back(clientSocketTemp);

		/* Display the signature of the new client ----- */

		memcpy(&clientIPAddress, &clientAddress.sin_addr.s_addr, 4);
		printf("\n*** Profile for a connecting client ***\n");
		printf("IP address: %s\n", inet_ntoa(clientIPAddress));
		printf("Client-side Port: %d\n", ntohs(clientAddress.sin_port));
		printf("\n");

		// Create the server socket, fill-in address information, and then connect
		serverSocketTemp = socket(AF_INET, SOCK_STREAM, 0);
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(SERVERPORT);
		serverAddress.sin_addr.s_addr = inet_addr(SERVERIP);
		connect(serverSocketTemp, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
		serverSocket.push_back(serverSocketTemp);

		int threadArgs[2] = { clientSocketPosition, serverSocketPosition };
		clientSocketPosition++;
		serverSocketPosition++;
		// Setting up arguments for the threads (easier to pass socket information) and then start threads
		if (_beginthread(clientToServer, 4096, (void *)threadArgs) < 0) {
			printf("ERROR - Unable to create client to server thread \n");
			exit(1);
		}
		if (_beginthread(serverToClient, 4096, (void *)threadArgs) < 0) {
			printf("ERROR - Unable to create client to server thread \n");
			exit(1);
		}
	}
	closesocket(proxySocket);
	WSACleanup();
	return 0;
}



void clientToServer(void *in_args) {
	unsigned int  fh, bufferLength, returnCode;
	char          inBuffer[BUFFERSIZE], outBuffer[BUFFERSIZE], *fileName;
	bool          connectionFlag = 1;
	int	clientSocketPosition, serverSocketPosition;

	clientSocketPosition = ((int *) in_args)[0];
	serverSocketPosition = ((int *) in_args)[1];

	returnCode = recv(clientSocket[clientSocketPosition], inBuffer, strlen(inBuffer), 0);
		//printf("The return code is %d and the in buffer says %s", returnCode, inBuffer);
		if (returnCode != -1) {
			if (strstr(inBuffer, hazardous_contents_CS_01) || strstr(inBuffer, hazardous_contents_CS_02)) {
				strcpy(outBuffer, NOTOK_401);
				send(clientSocket[clientSocketPosition], outBuffer, strlen(outBuffer), 0);
				strcpy(outBuffer, MESS_401);
				send(clientSocket[clientSocketPosition], outBuffer, strlen(outBuffer), 0);
				closesocket(serverSocket[serverSocketPosition]);
				closesocket(clientSocket[clientSocketPosition]);
			}
			if (serverSocket[serverSocketPosition] == 0) {
				strcpy(outBuffer, NOTOK_404);
				send(clientSocket[clientSocketPosition], outBuffer, strlen(outBuffer), 0);
				strcpy(outBuffer, MESS_404);
				send(clientSocket[clientSocketPosition], outBuffer, strlen(outBuffer), 0);
				connectionFlag = 0;
			}
			else {
				//printf("%s", inBuffer);
				strcpy(outBuffer, inBuffer);
				send(serverSocket[serverSocketPosition], outBuffer, returnCode, 0);
			}
	}
	_endthread();
}


void serverToClient(void *in_args) {
	unsigned int  fh, bufferLength, returnCode;
	bool          connectionFlag = 1;
	int	clientSocketPosition, serverSocketPosition;
	clientSocketPosition = ((int *)in_args)[0];
	serverSocketPosition = ((int *)in_args)[1];

	while (1) {
		char inBuffer[BUFFERSIZE];
		char outBuffer[BUFFERSIZE];
		returnCode = recv(serverSocket[serverSocketPosition], inBuffer, BUFFERSIZE, 0);
		if (returnCode == -1) {
			printf("Error occured with server connection");
			break;
		}
		else if(returnCode == 0){
			printf("Disconnected from server");
			break;
		}
		else {
			if (strstr(inBuffer, hazardous_contents_SC_01) || strstr(inBuffer, hazardous_contents_SC_02)) {
				strcpy(outBuffer, NOTOK_401);
				send(clientSocket[clientSocketPosition], outBuffer, strlen(outBuffer), 0);
				strcpy(outBuffer, MESS_401);
				send(clientSocket[clientSocketPosition], outBuffer, strlen(outBuffer), 0);
				break;
			}
			send(clientSocket[clientSocketPosition], inBuffer, returnCode, 0);
		}
	}
	closesocket(serverSocket[serverSocketPosition]);
	closesocket(clientSocket[clientSocketPosition]);
	_endthread();
}
