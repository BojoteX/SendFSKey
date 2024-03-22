#include <winsock2.h>
#include "NetworkServer.h"

// Global variables or resources for the server
SOCKET g_serverSocket = INVALID_SOCKET;

bool initializeServer() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void shutdownServer() {
    // Implement server shutdown code here
    // Close server socket, release resources, etc.
    closesocket(g_serverSocket);
}

SOCKET acceptConnection() {
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8028);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    SOCKET clientSocket = accept(listenSocket, NULL, NULL);
    closesocket(listenSocket); // Close listening socket after accepting connection
    return clientSocket;
}

bool sendData(SOCKET clientSocket, const char* data, int dataSize) {
    return send(clientSocket, data, dataSize, 0) != SOCKET_ERROR;
}

bool receiveData(SOCKET clientSocket, char* buffer, int bufferSize) {
    int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
    return bytesReceived != SOCKET_ERROR;
}

void closeConnection(SOCKET clientSocket) {
    closesocket(clientSocket);
}

void cleanupServer() {
    WSACleanup();
}