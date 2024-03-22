#pragma once

SOCKET acceptConnection();
bool sendData(SOCKET clientSocket, const char* data, int dataSize);
bool receiveData(SOCKET clientSocket, char* buffer, int bufferSize);
void closeConnection(SOCKET clientSocket);
void cleanupServer();
bool initializeServer();
void shutdownServer();