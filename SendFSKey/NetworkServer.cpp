#include <ws2tcpip.h>
#include <thread>
#include <stdexcept>
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

#include <Windows.h>
#include <cstdio> // For printf

void handleClient(SOCKET clientSocket) {
    // Adjust the buffer size to exactly fit our data structure: 1 byte for event type + 4 bytes for keyCode
    unsigned char buffer[5];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        // Assuming receiveData is properly defined elsewhere to wrap recv() and return true if data was received
        int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving data or client disconnected.\n");
            break; // Exit loop on error or disconnection
        }

        // First byte is the event type
        char eventType = buffer[0];

        // Next 4 bytes are the keyCode, ensure proper alignment and endianness
        UINT keyCode;
        memcpy(&keyCode, buffer + 1, sizeof(keyCode));

        switch (eventType) {
        case 'D':
            printf("Key Down: %u\n", keyCode);
            SendKeyPressDOWN(keyCode);
            break;
        case 'U':
            printf("Key Up: %u\n", keyCode);
            SendKeyPressUP(keyCode);
            break;
        default:
            printf("Unknown event type: %c\n", eventType);
            break; // Optionally, handle unknown event type
        }

        // Send acknowledgment back to the client
        char ack = 1;
        send(clientSocket, &ack, sizeof(ack), 0);
    }
    printf("Client socket closed.\n");
    closesocket(clientSocket); // Clean up the socket
}

void handleClientOLDOLD(SOCKET clientSocket) {
    char buffer[1024];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        if (!receiveData(clientSocket, buffer, sizeof(buffer))) {
            printf("Error receiving data or client disconnected.\n");
            continue; // Keep the server running even in case of error
        }

        // Ensure the buffer contains at least 2 characters (1 for event type, 1 for keyCode)
        if (strlen(buffer) < 2) {
            printf("Received data is too short or incorrect format.\n");
            // Optionally, send an error message back to client
            continue; // Skip to the next iteration of the loop without closing the socket
        }

        char eventType = buffer[0];
        // Attempt to convert the remaining part of the buffer to an integer
        int keyCode;
        try {
            keyCode = std::stoi(buffer + 1);
        }
        catch (const std::invalid_argument& ia) {
            printf("Invalid key code received: %s\n", ia.what());
            continue;
        }
        catch (const std::out_of_range& oor) {
            printf("Key code out of range: %s\n", oor.what());
            continue;
        }

        switch (eventType) {
        case 'D':
            printf("Key Down: %d\n", keyCode);
            SendKeyPressDOWN(keyCode);
            break;
        case 'U':
            printf("Key Up: %d\n", keyCode);
            SendKeyPressUP(keyCode);
            break;
        default:
            printf("Unknown event type: %c\n", eventType);
            // Handle unknown event type, if necessary
            break;
        }

        // Acknowledge the received message
        char ack = 1;
        sendData(clientSocket, &ack, sizeof(ack));
    }
    // Move the socket closure outside of the while loop
    printf("Client socket closed.\n");
    closesocket(clientSocket); // Ensure socket is closed when the server is no longer running
}

void handleClientNEW(SOCKET clientSocket) {
    char buffer[1024];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        // Assume receiveData is a wrapper around recv() that returns true if data was received
        if (!receiveData(clientSocket, buffer, sizeof(buffer))) {
            printf("Error receiving data or client disconnected.\n");
            break; // Exit the loop if there's an error or disconnection
        }

        // First character of the buffer is the event type: 'D' for key down, 'U' for key up
        char eventType = buffer[0];

        // The rest of the buffer is the key code
        // Convert the key code part of the buffer to an integer
        int keyCode = atoi(buffer + 1); // Skip the first character when converting

        if (eventType == 'D') {
            printf("Key Down: %d\n", keyCode);
            SendKeyPressDOWN(keyCode);
        }
        else if (eventType == 'U') {
            printf("Key Up: %d\n", keyCode);
            SendKeyPressUP(keyCode);
        }
        else {
            printf("Unknown event type: %c\n", eventType);
            // Handle unknown event type, if necessary
        }

        // Acknowledge the received message
        char ack = 1;
        sendData(clientSocket, &ack, sizeof(ack));
    }
    closesocket(clientSocket); // Clean up the socket
    printf("Client socket closed.\n");
}

void handleClientOLD(SOCKET clientSocket) {
    char buffer[1024];
    while (serverRunning) {
        ZeroMemory(buffer, sizeof(buffer));
        if (!receiveData(clientSocket, buffer, sizeof(buffer))) {
            printf("Error receiving data or client disconnected.\n");
            break;
        }

        printf("Received data: %s\n", buffer);

        // Convert char* (narrow string) to std::wstring (wide string)
        int bufferSize = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, nullptr, 0);
        std::wstring wideBuffer(bufferSize, 0);
        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, &wideBuffer[0], bufferSize);

        // Remove the null terminator added by MultiByteToWideChar
        if (!wideBuffer.empty()) {
            wideBuffer.pop_back();
        }

        std::wstring formattedMessage = L"Keypress received (" + wideBuffer + L") - \n";
        AppendTextToConsole(hEdit, formattedMessage.c_str());

        // Convert wideBuffer to an integer
        int keyCode = std::stoi(wideBuffer);

        // Try to send the KeyCode to MS Flight Simulator 2020
        SendKeyPressUP(keyCode);
        std::wstring formattedMessageOut = L"Simulated Keypress sent (" + std::to_wstring(keyCode) + L")\r\n";
        AppendTextToConsole(hEdit, formattedMessageOut.c_str());

        char ack = 1; // Send an acknowledgment back to the client
        sendData(clientSocket, &ack, sizeof(ack));
    }
    closesocket(clientSocket); // Close the client socket
    printf("Client socket closed.\n");
}

void startServer() {
    serverRunning = true;

    // This loop runs by every single client connection (not client interaction and data payloads from clients, for that see handleClient above)
    while (serverRunning) {

        // Get the IP Address of the connecting client
        sockaddr_in clientAddr; // Declare the structure to store client address
        int clientAddrLen = sizeof(clientAddr); // Length of client address

        // SOCKET clientSocket = accept(g_listenSocket, NULL, NULL);
        SOCKET clientSocket = accept(g_listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            printf("Failed to accept connection.\n");
            continue; // Continue to accept the next connection
        }

        // Use inet_ntop correctly
        char clientIPStr[INET_ADDRSTRLEN]; // Buffer where the IP string will be stored
        if (inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIPStr, INET_ADDRSTRLEN) == NULL) {
            printf("Failed to convert IP address.\n");
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