#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include "Globals.h"
#include "NetworkServer.h"

std::atomic<bool> serverRunning;
SOCKET g_listenSocket = INVALID_SOCKET;

HWND hEdit; // Declare hEdit globally
void AppendTextToConsole(HWND hEdit, const wchar_t* text);

std::wstring getServerIPAddress() {
    wchar_t hostname[NI_MAXHOST] = L"";
    std::wstring ipAddress = L"";

    // Use GetComputerNameW for Unicode hostname retrieval
    DWORD hostnameLen = NI_MAXHOST;
    if (!GetComputerNameW(hostname, &hostnameLen)) {
        wprintf(L"GetComputerNameW failed with error: %d\n", GetLastError());
        return L"";
    }

    struct addrinfo hints, * res = NULL, * ptr = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // For IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE; // Use my IP

    // Convert hostname to narrow string for getaddrinfo
    char narrowHostname[NI_MAXHOST] = "";
    WideCharToMultiByte(CP_ACP, 0, hostname, -1, narrowHostname, NI_MAXHOST, NULL, NULL);

    int result = getaddrinfo(narrowHostname, NULL, &hints, &res);
    if (result != 0) {
        wprintf(L"getaddrinfo failed with error: %d\n", result);
        return L"";
    }

    // Loop through all the results and convert the first found IPv4 address to a string
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET) { // Check it is IPv4
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);

            // Convert narrow string IP to wide string
            wchar_t wideIpStr[INET_ADDRSTRLEN];
            MultiByteToWideChar(CP_ACP, 0, ipStr, -1, wideIpStr, INET_ADDRSTRLEN);
            ipAddress = wideIpStr;
            break;
        }
    }

    freeaddrinfo(res);
    return ipAddress;
}

bool initializeServer() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }

    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all network interfaces
    serverAddr.sin_port = htons(port);

    if (bind(g_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(g_listenSocket);
        WSACleanup();
        return false;
    }

    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(g_listenSocket);
        WSACleanup();
        return false;
    }

    return true;
}

void shutdownServer() {
    closesocket(g_listenSocket);
    WSACleanup();
}

bool sendData(SOCKET clientSocket, const char* data, int dataSize) {
    return send(clientSocket, data, dataSize, 0) != SOCKET_ERROR;
}

bool receiveData(SOCKET clientSocket, char* buffer, int bufferSize) {
    int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
    return bytesReceived != SOCKET_ERROR;
}

void closeServerConnection(SOCKET clientSocket) {
    closesocket(clientSocket);
}

void cleanupServer() {
    shutdownServer(); // Ensure the server is properly shut down
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        if (!receiveData(clientSocket, buffer, sizeof(buffer))) {
            if (DEBUG) printf("Error receiving data or client disconnected.\n");
            break;
        }

        if (DEBUG) printf("Received data: %s\n", buffer);
        AppendTextToConsole(hEdit, L"Server initialized successfully!\n");

        char ack = 1; // Send an acknowledgment back to the client
        sendData(clientSocket, &ack, sizeof(ack));
    }
    closesocket(clientSocket); // Close the client socket
    if (DEBUG) printf("Client socket closed.\n");
}

void startServer() {
    serverRunning = true;
    while (serverRunning) {
        SOCKET clientSocket = accept(g_listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (DEBUG) printf("Failed to accept connection.\n");
            continue; // Continue to accept the next connection
        }

        std::thread serverThread(handleClient, clientSocket);
        serverThread.detach(); // Detach the thread to handle the client independently
    }
}