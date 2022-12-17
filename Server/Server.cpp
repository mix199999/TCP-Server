
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#pragma comment (lib, "Ws2_32.lib")
#include<iostream>
#include <vector>
#include <map>
#include <string>
#include <fcntl.h>
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "2137"
#define MAX_CLIENTS 10

using namespace std;

//function to receive message from the client and display it in the console
void receive_message_display(SOCKET socket, char* message, int iResult)
{

	message[iResult] = '\0';
	printf("Message: %s \r\n", message);

}


//main function
int main()
{
	//WSADATA structure	

	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;


	struct addrinfo* result = NULL;
	struct addrinfo hints;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;





	//Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;



	//Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\r\n", iResult);
		WSACleanup();
		return 1;
	}


	//Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\r\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\r\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//Releasing the memory
	freeaddrinfo(result);



	//Listen for incoming connections
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\r\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	printf("Waiting for connection... \r\n");


	struct sockaddr_storage client_addr;
	socklen_t client_addr_len = sizeof(client_addr);


	//Creating containers for storing clients and their addresses pointers to the files etc.
	vector<pollfd> socketList;
	map<int, FILE*> clientFiles;
	map<int, string> clientAddresses;

	int poll;
	int polled = 0;
	char fileName[INET_ADDRSTRLEN + 3];
	uint8_t cnt = 0;

	//Adding server socket to the socket list
	pollfd server;
	server.fd = ListenSocket;
	server.events = POLLIN;

	socketList.push_back(server);

	//Infinite loop for handling connections
	while (true)
	{
		char* welcomeMessage = (char*)"Welcome to the server list of commands: \r\n 1. download  \r\n 2. upload \r\n 3. exit \r\n";
		// Polling
		poll = WSAPoll(socketList.data(), socketList.size(), -1);
		if (poll == -1)
		{
			perror("Cannot pull sockets");
			exit(EXIT_FAILURE);
		}
		// Loop through all connections
		for (int i = 0; i < socketList.size(); i++)
		{

			// If the connection is ready to be read from
			if (socketList[i].revents & POLLIN)
			{

				// If the connection is the server's socket
				if (socketList[i].fd == ListenSocket)
				{
					// Accept the connection
					int client_socket = accept(ListenSocket, (struct sockaddr*)&client_addr, &client_addr_len);

					if (client_socket == -1)
					{
						perror("Cannot accept connection\r\n");
						exit(EXIT_FAILURE);
					}

					// Add the connection to the sockets vector
					server.fd = client_socket;
					server.events = POLLIN;
					socketList.push_back(server);
					// Create a file for the connection
					sprintf(fileName, "%d", cnt);
					clientFiles[client_socket] = fopen(fileName, "w");
					// Get the address of the connection
					if (client_addr.ss_family == AF_INET)
					{
						struct sockaddr_in* s = (struct sockaddr_in*)&client_addr;
						inet_ntop(AF_INET, &s->sin_addr, fileName, INET_ADDRSTRLEN);
					}

					clientAddresses[client_socket] = fileName;
					cnt++;
				}

				//If the connection is a client's socket
				else
				{
					//Creating a buffer for storing the message
					char buf[DEFAULT_BUFLEN];

					int bytes_read = recv(socketList[i].fd, buf, DEFAULT_BUFLEN, 0);
					// If the connection was closed

					if (bytes_read <= 0)
					{
						// Close the connection
						closesocket(socketList[i].fd);
						// Remove the connection from the vector
						socketList.erase(socketList.begin() + i);
						// Close the file
						fclose(clientFiles[socketList[i].fd]);
						// Remove the file from the map
						clientFiles.erase(socketList[i].fd);
						// Remove the address from the map
						clientAddresses.erase(socketList[i].fd);
						// Decrement the counter
						cnt--;
						// Go to the next connection
						continue;
					}


					//Displaying the message from the client
					receive_message_display(socketList[i].fd, buf, bytes_read);


					//Command for uploading a file
					if (strcmp(buf, "upload") == 0)
					{

						char fileBuf[DEFAULT_BUFLEN];


						char* uploadMessage = (char*)"\r\nEnter the name of the file you want to upload: \r\n";
						send(socketList[i].fd, uploadMessage, strlen(uploadMessage), 0);
						char* filename = (char*)malloc(100);
						int bytes = recv(socketList[i].fd, filename, 100, 0);

						FILE* fp;


						printf("C:/cos/%s-%d.txt", clientAddresses[socketList[i].fd].c_str(), cnt);


						sprintf(filename, "C:/cos/%s-%d.txt", clientAddresses[socketList[i].fd].c_str(), cnt);

						fp = fopen(filename, "w");
						if (fp == NULL)
						{
							printf("Cannot create file \n");
							continue;
						}

						bytes = recv(socketList[i].fd, fileBuf, DEFAULT_BUFLEN, 0);


						fileBuf[bytes] = '\0';
						fprintf(fp, "%s", fileBuf);
						fclose(fp);
						printf("File received successfully \n");

						//tell client that the file was received successfully
						char* success = (char*)"\r\nFile received successfully \n";
						send(socketList[i].fd, success, strlen(success), 0);


					}

					//Command for downloading a file
					else if (strcmp(buf, "download") == 0)

					{

						//tell client to sent message with path to the file
						char* downloadMessage = (char*)"\r\nEnter the name of the file you want to download: \r\n";
						send(socketList[i].fd, downloadMessage, strlen(downloadMessage), 0);
						char* filename = (char*)malloc(100);
						int bytes = recv(socketList[i].fd, filename, 100, 0);
						filename[bytes] = '\0';
						FILE* fp;
						fp = fopen(filename, "r");
						if (fp == NULL)
						{
							printf("Cannot open file \n");
							continue;
						}

						char fileBuf[DEFAULT_BUFLEN];
						while (fgets(fileBuf, DEFAULT_BUFLEN, fp) != NULL)
						{
							send(socketList[i].fd, fileBuf, strlen(fileBuf), 0);
						}
						fclose(fp);
						printf("File sent successfully \n");

						//tell client that the file was sent successfully
						char* success = (char*)"\r\nFile sent successfully \n";
						send(socketList[i].fd, success, strlen(success), 0);


					}

					else
					{
						//send to the client wrong command message
						char* wrongCommandMessage = (char*)"\r\nWrong command. Try again.\r\n";
						send(socketList[i].fd, wrongCommandMessage, strlen(wrongCommandMessage), 0);
						//send to client list of commands
						char* commandsMessage = (char*)"List of commands:\r\nupload,download\r\n";
						send(socketList[i].fd, commandsMessage, strlen(commandsMessage), 0);
						//tell client to don not use enter button instead use space
						char* enterMessage = (char*)"Do not use enter button. Use ctrl+d instead.\r\n";
						send(socketList[i].fd, enterMessage, strlen(enterMessage), 0);

					}

				}
				// Increment the counter of handled connections
				polled++;
			}
			// If all connections were handled
			if (polled == poll)
			{
				// Break the loop
				break;
			}
		}
		// Reset the counter
		polled = 0;
	}


	// cleanup
	closesocket(ListenSocket);
	WSACleanup();
}

