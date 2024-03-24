#include <ws2tcpip.h>
#include <sstream>
#include <thread>
#include "Globals.h"
#include "NetworkServer.h"

std::atomic<bool> serverRunning;
SOCKET g_listenSocket = INVALID_SOCKET;

std::wstring getServerIPAddress() {
    wchar_t hostname[NI_MAXHOST] = L"";
    std::wstring ipAddress = L"";

    // Use GetComputerNameW for Unicode hostname retrieval
    DWORD hostnameLen = NI_MAXHOST;
    if (!GetComputerNameW(hostname, &hostnameLen)) {
        wprintf(L"GetComputerNameW failed with error: %d\n", GetLastError());
        Log(L"GetCpmputerNameW failed.");
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
        Log(L"getaddrinfo failed");
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
        Log(L"WSASStartup failed");
        return false;
    }

    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        Log(L"Error at socket()");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all network interfaces
    serverAddr.sin_port = htons(port);

    if (bind(g_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        Log(L"Bind failed with error");
        closesocket(g_listenSocket);
        WSACleanup();
        return false;
    }

    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        Log(L"Listen failed");
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

/*
void handleClient(SOCKET clientSocket) {
    // Adjust the buffer size to exactly fit our data structure: 1 byte for event type + 2 bytes for scanCode
    unsigned char buffer[3];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving data or client disconnected.\n");
            Log(L"Error receiving data or client disconnected");
            break; // Exit loop on error or disconnection
        }

        // First byte is the event type
        char eventType = buffer[0];

        // Next 2 bytes are the scanCode, ensure proper alignment and endianness
        UINT scanCode;
        memcpy(&scanCode, buffer + 1, sizeof(scanCode));

        // Assuming scan codes are being sent as 2 bytes, adjust if necessary
        scanCode = ntohs(scanCode); // Convert from network byte order to host byte order if needed

        switch (eventType) {
        case 'D':
            Log(L"KEY_DOWN RECEIVED BY SERVER: " + std::to_wstring(scanCode));
            printf("Key Down: %u\n", scanCode);
            SendKeyPressDOWN(scanCode); // Now expecting a scanCode
            break;
        case 'U':
            Log(L"KEY_UP RECEIVED BY SERVER: " + std::to_wstring(scanCode));
            printf("Key Up: %u\n", scanCode);
            SendKeyPressUP(scanCode); // Now expecting a scanCode
            break;
        default:
            Log(L"UNKNOWN EVENT TYPE RECEIVED BY SERVER: " + std::to_wstring(scanCode));
            printf("Unknown event type: %c\n", eventType);
            break; // Optionally, handle unknown event type
        }

        // Send acknowledgment back to the client
        char ack = 1;
        send(clientSocket, &ack, sizeof(ack), 0);
    }
    printf("Client socket closed.\n");
    closesocket(clientSocket); // Clean up the socket
    Log(L"Client socket closed");
}
*/


void handleClient(SOCKET clientSocket) {
    // Adjust the buffer size to exactly fit our data structure: 1 byte for event type + 4 bytes for keyCode
    unsigned char buffer[5];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        // Assuming receiveData is properly defined elsewhere to wrap recv() and return true if data was received
        int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving data or client disconnected.\n");
            Log(L"Error receiving data or client disconnected");
            break; // Exit loop on error or disconnection
        }

        // First byte is the event type
        char eventType = buffer[0];

        // Next 4 bytes are the keyCode, ensure proper alignment and endianness
        UINT keyCode;
        memcpy(&keyCode, buffer + 1, sizeof(keyCode));

        switch (eventType) {
        case 'D':
            Log(L"KEY_DOWN RECEIVED BY SERVER: " + keyCode);
            printf("\nKey Down RECEIVED: %u\n", keyCode);
            SendKeyPressDOWN(keyCode);
            break;
        case 'U':
            Log(L"KEY_UP RECEIVED BY SERVER: " + keyCode);
            printf("Key Up RECEIVED: %u\n", keyCode);
            SendKeyPressUP(keyCode);
            break;
        default:
            Log(L"UNKNOWN_KEY RECEIVED BY SERVER: " + keyCode);
            printf("Unknown event RECEIVED of type: %c\n", eventType);
            break; // Optionally, handle unknown event type
        }

        // Send acknowledgment back to the client
        char ack = 1;
        send(clientSocket, &ack, sizeof(ack), 0);
    }
    printf("Client socket closed.\n");
    closesocket(clientSocket); // Clean up the socket
    Log(L"Client socket closed");
}

void startServer() {
    serverRunning = true;

    // This loop runs by every single client connection (not client interaction and data payloads from clients, for that see handleClient above)
    while (serverRunning) {

        // Get the IP Address of the connecting client
        sockaddr_in clientAddr; // Declare the structure to store client address
        int clientAddrLen = sizeof(clientAddr); // Length of client address

        SOCKET clientSocket = accept(g_listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            printf("Failed to accept connection.\n");
            Log(L"Failed to accept connection");
            continue; // Continue to accept the next connection
        }

        /*
        // Set a timeout for recv operations
        struct timeval timeout;
        timeout.tv_sec = 5;  // Timeout after 5 seconds
        timeout.tv_usec = 0; // 0 microseconds

        if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            printf("Failed to set socket timeout.\n");
            // Decide how you want to handle this error. You might close the socket, log the error, etc.
            closesocket(clientSocket);
            Log(L"Closed socket for unresponsive client");
            continue;
        }
        */

        // Use inet_ntop correctly
        char clientIPStr[INET_ADDRSTRLEN]; // Buffer where the IP string will be stored
        if (inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIPStr, INET_ADDRSTRLEN) == NULL) {
            printf("Failed to convert IP address.\n");
            Log(L"Failed to convert IP address");
            continue;
        }

        std::string ipStr(clientIPStr); // Convert char* to std::string
        std::wstring wideIP(ipStr.begin(), ipStr.end()); // Convert std::string to std::wstring for UI

        std::wstring formattedMessage = L"Received connection from: " + wideIP + L"\r\n";

        // PostUpdateToUI(formattedMessage.c_str());
        AppendTextToConsole(hEdit, formattedMessage.c_str());

        std::thread serverThread(handleClient, clientSocket);
        serverThread.detach(); // Detach the thread to handle the client independently
    }
}