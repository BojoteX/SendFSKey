#pragma once

bool initializeServer();
bool sendData(SOCKET clientSocket, const char* data, int dataSize);
bool receiveData(SOCKET clientSocket, char* buffer, int bufferSize);
void closeServerConnection(SOCKET clientSocket);
void cleanupServer();
void shutdownServer();
void startServer();
std::wstring getServerIPAddress();
bool isServerUp();
void serverStartThread();