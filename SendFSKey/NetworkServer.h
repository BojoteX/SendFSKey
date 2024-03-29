#pragma once

#include "Globals.h"

bool initializeServer();
bool sendData(SOCKET clientSocket, const char* data, int dataSize);
bool receiveData(SOCKET clientSocket, char* buffer, int bufferSize);
void closeServerConnection(SOCKET clientSocket);