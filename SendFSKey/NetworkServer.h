#pragma once

#include "Globals.h"

bool sendData(SOCKET clientSocket, const char* data, int dataSize);
bool receiveData(SOCKET clientSocket, char* buffer, int bufferSize);
void cleanupServer();
void startServer();
bool initializeServer();
void closeServerConnection(SOCKET clientSocket);
std::wstring getServerIPAddress();