#include "Utils.h"

bool addSocket(SOCKET id, int what, SocketState* sockets, int& socketsCount)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].activityTime = time(0);
			sockets[i].dataLen = 0;
			socketsCount++;
			return true;
		}
	}

	return false;
}

void removeSocket(int index, SocketState* sockets, int& socketsCount)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].activityTime = 0;
	socketsCount--;
	cout << "The socket number " << index << " has been removed" << endl;
}

void acceptConnection(int index, SocketState* sockets, int& socketsCount)
{
	SOCKET id = sockets[index].id;
	sockets[index].activityTime = time(0);
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	// Set the socket to be in non-blocking mode.
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "HTTP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE, sockets, socketsCount) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}

	return;
}

void receiveMessage(int index, SocketState* sockets, int& socketsCount)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].dataLen;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index, sockets, socketsCount);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index, sockets, socketsCount);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; 
		cout << "HTTP Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";
		sockets[index].dataLen += bytesRecv;

		if (sockets[index].dataLen > 0)
		{
			if (strncmp(sockets[index].buffer, "GET", 3) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = GET;
				strcpy(sockets[index].buffer, &sockets[index].buffer[5]);
				sockets[index].dataLen = strlen(sockets[index].buffer);
				sockets[index].buffer[sockets[index].dataLen] = NULL;
				return;
			}
			else if (strncmp(sockets[index].buffer, "POST", 4) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = POST;
				strcpy(sockets[index].buffer, &sockets[index].buffer[6]);
				sockets[index].dataLen = strlen(sockets[index].buffer);
				sockets[index].buffer[sockets[index].dataLen] = NULL;
				return;
			}
			else if (strncmp(sockets[index].buffer, "HEAD", 4) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = HEAD;
				strcpy(sockets[index].buffer, &sockets[index].buffer[6]);
				sockets[index].dataLen = strlen(sockets[index].buffer);
				sockets[index].buffer[sockets[index].dataLen] = NULL;
				return;
			}
			else if (strncmp(sockets[index].buffer, "PUT", 3) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = PUT;
				return;
			}
			else if (strncmp(sockets[index].buffer, "DELETE", 6) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = HDELETE;
				return;
			}
			else if (strncmp(sockets[index].buffer, "TRACE", 5) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = TRACE;
				strcpy(sockets[index].buffer, &sockets[index].buffer[5]);
				sockets[index].dataLen = strlen(sockets[index].buffer);
				sockets[index].buffer[sockets[index].dataLen] = NULL;
				return;
			}
			else if (strncmp(sockets[index].buffer, "OPTIONS", 7) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].reqType = OPTIONS;
				strcpy(sockets[index].buffer, &sockets[index].buffer[9]);
				sockets[index].dataLen = strlen(sockets[index].buffer);
				sockets[index].buffer[sockets[index].dataLen] = NULL;
				return;
			}
			
		}
	}
}


bool sendMessage(int index, SocketState* sockets)
{
	int bytesSent = 0, buffLen = 0, fileSize = 0;
	char sendBuff[BUFF_SIZE];
	char* tempFromTok;
	char tempBuff[BUFF_SIZE], readBuff[BUFF_SIZE];
	string respone, fileSizeString, localFile;
	ifstream file;
	time_t currentTime;
	time(&currentTime); // Get current time
	SOCKET msgSocket = sockets[index].id;
	sockets[index].activityTime = time(0); 

	switch (sockets[index].reqType)
	{
		case HEAD:
		{
			tempFromTok = strtok(sockets[index].buffer, " ");
			localFile = "C:\\Server\\indexen.html"; 
			file.open(localFile);
			if (!file)
			{
				respone = "HTTP/1.1 404 Not Found ";
				fileSize = 0;
			}
			else
			{
				respone = "HTTP/1.1 200 OK ";
				file.seekg(0, ios::end);
				fileSize = file.tellg(); // get length of content in file
			}

			respone += "\r\nContent-type: text/html";
			respone += "\r\nDate:";
			respone += ctime(&currentTime);
			respone += "Content-length: ";
			fileSizeString = to_string(fileSize);
			respone += fileSizeString;
			respone += "\r\n\r\n";
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			file.close();
			break;
		}

		case GET:
		{
			string textFromFile = "";
			tempFromTok = strtok(sockets[index].buffer, " ");
			localFile = "C:\\Server\\index"; 
			char* langPtr = strchr(tempFromTok, '?'); 
			if (langPtr == NULL) {
				localFile += "en.html";
			}
			else{
				langPtr += 6; 
				localFile += *langPtr;
				localFile += *(langPtr+1);
				localFile += ".html";
			}

			
			file.open(localFile);
			if (!file){
				respone = "HTTP/1.1 404 Not Found ";
				file.open("C:\\Server\\error.html"); 
			}
			else{
				respone = "HTTP/1.1 200 OK ";
			}

			if (file){
				while (file.getline(readBuff, BUFF_SIZE))
				{
					textFromFile += readBuff;
					fileSize += strlen(readBuff);
				}
			}

			respone += "\r\nContent-type: text/html";
			respone += "\r\nDate:";
			respone += ctime(&currentTime);
			respone += "Content-length: ";
			fileSizeString = to_string(fileSize);
			respone += fileSizeString;
			respone += "\r\n\r\n";
			respone += textFromFile;
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			file.close();
			break;
		}

		case PUT:
		{
			char fileName[BUFF_SIZE];
			int returnCode = putHandler(index, fileName, sockets);
			switch (returnCode)
			{
				case 0:
					cout << "PUT " << fileName << "Failed";
					respone = "HTTP/1.1 412 Precondition failed \r\nDate: ";
					break;
				

				case 200:
					respone = "HTTP/1.1 200 OK \r\nDate: ";
					break;
				

				case 201:
					respone = "HTTP/1.1 201 Created \r\nDate: ";
					break;
				

				case 204:
					respone = "HTTP/1.1 204 No Content \r\nDate: ";
					break;

				default:
					respone = "HTTP/1.1 501 Not Implemented \r\nDate: ";
					break;
				
			}

			respone += ctime(&currentTime);
			respone += "Content-length: ";
			fileSizeString = to_string(fileSize);
			respone += fileSizeString;
			respone += "\r\n\r\n";
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			break;
		}

		case HDELETE:
		{
			strtok(&sockets[index].buffer[8], " ");
			strcpy(tempBuff, &sockets[index].buffer[8]);
			if (remove(tempBuff) != 0)
			{
				respone = "HTTP/1.1 204 No Content \r\nDate: "; // We treat 204 code as a case where delete wasn't successful
			}
			else
			{
				respone = "HTTP/1.1 200 OK \r\nDate: "; // File deleted succesfully
			}

			respone += ctime(&currentTime);
			respone += "Content-length: ";
			fileSizeString = to_string(fileSize);
			respone += fileSizeString;
			respone += "\r\n\r\n";
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			break;
		}

		case TRACE:
		{
			fileSize = strlen("TRACE");
			fileSize += strlen(sockets[index].buffer);
			respone = "HTTP/1.1 200 OK \r\nContent-type: message/http\r\nDate: ";
			respone += ctime(&currentTime);
			respone += "Content-length: ";
			fileSizeString = to_string(fileSize);
			respone += fileSizeString;
			respone += "\r\n\r\n";
			respone += "TRACE";
			respone += sockets[index].buffer;
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			break;
		}

		case OPTIONS:
		{
			respone = "HTTP/1.1 204 No Content\r\nAllow: OPTIONS, GET, HEAD, POST, PUT, TRACE, DELETE\r\n";
			respone += "Content-length: 0\r\n\r\n";
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			break;
		}

		case POST:
		{
			respone = "HTTP/1.1 200 OK \r\nDate:";
			respone += ctime(&currentTime);
			respone += "Content-length: 0\r\n\r\n";
			char* messagePtr = strstr(sockets[index].buffer, "\r\n\r\n");
			cout << "Body of POST Request : "<< messagePtr + 4 << endl;
			buffLen = respone.size();
			strcpy(sendBuff, respone.c_str());
			break;
		}
	}

	bytesSent = send(msgSocket, sendBuff, buffLen, 0);
	memset(sockets[index].buffer, 0, BUFF_SIZE);
	sockets[index].dataLen = 0;
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return false;
	}

	cout << "HTTP Server: Sent: " << bytesSent << "\\" << buffLen << " bytes of \n \"" << sendBuff << "\"\message.\n";
	sockets[index].send = IDLE;
	return true;
}

int putHandler(int index, char* filename, SocketState* sockets)
{
	char* tempPtr = 0;
	int buffLen = 0;
	int statusCode = 200; 
	tempPtr = strtok(&sockets[index].buffer[5], " ");
	strcpy(filename, &sockets[index].buffer[5]);
	fstream outPutFile;
	outPutFile.open(filename);

	if (!outPutFile.good()){
		outPutFile.open(filename, ios::out);
		statusCode = 201; 
	}

	if (!outPutFile.good()){
		cout << "HTTP Server: Error writing file to local storage: " << WSAGetLastError() << endl;
		return 0; 
	}

	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, ":");
	tempPtr = strtok(NULL, "\r\n");
	tempPtr = strtok(NULL, "\r\n");
	tempPtr += (strlen(tempPtr) + 1);


	
	if (tempPtr == 0){
		statusCode = 204; 
	}
	else{//copy the body to the file
		while (*tempPtr != '\0')
		{
			outPutFile << tempPtr;
			tempPtr += (strlen(tempPtr) + 1);
		}
	}
	

	outPutFile.close();
	return statusCode;
}
