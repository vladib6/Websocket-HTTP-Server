#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

const int PORT = 27015;
const int MAX_SOCKETS = 60;
const int BUFF_SIZE = 1024;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int GET = 5;
const int HEAD = 6;
const int PUT = 7;
const int POST = 8;
const int HDELETE= 9;
const int TRACE = 10;
const int OPTIONS = 11;

struct SocketState
{
	SOCKET					id;
	int		                recv;
	int		                send;
	int		                reqType;
	char					buffer[BUFF_SIZE];
	time_t					activityTime;
	int						dataLen;
};


bool addSocket(SOCKET id, int what, SocketState* sockets, int& socketsCount);
void removeSocket(int index, SocketState* sockets, int& socketsCount);
void acceptConnection(int index, SocketState* sockets, int& socketsCount);
void receiveMessage(int index, SocketState* sockets, int& socketsCount);
bool sendMessage(int index, SocketState* sockets);
int PutRequest(int index, char* filename, SocketState* sockets);