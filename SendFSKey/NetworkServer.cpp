#include <thread>
#include <ws2tcpip.h>
#include "NetworkServer.h"

// Define the expected server signature
const char* SERVER_SIGNATURE = "SendFSKeySSv1";

SOCKET g_listenSocket = INVALID_SOCKET;

std::atomic<bool> serverRunning;

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

    freeaddrinfo(res); // Free the memory allocated by getaddrinfo
    return ipAddress;
}

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

        // Potentially handle keyCodes being sent not matching the expected values (e.g. out of range) and not between 0 and 255
        // For Example, I do not want codes 16, 17 or 18 being sent, but I can't intercept them here as I have no way to know if LEFT or RIGHT extended key was press
        // for that I need to make this change in the Client while in the loop    

        switch (eventType) {
        case 'D':
            printf("\n[KEY_DOWN] RECEIVED FROM CLIENT: (%u)\n", keyCode);
            ServerKeyPressDOWN(keyCode);
            break;
        case 'U':
            printf("[KEY_UP] RECEIVED FROM CLIENT: (%u)\n", keyCode);
            ServerKeyPressUP(keyCode);
            break;
        default:
            printf("[UNKNOWN] RECEIVED of type: %c\n", eventType);
            break; // Optionally, handle unknown event type
        }

        // Send acknowledgment back to the client
        char ack = 1;
        send(clientSocket, &ack, sizeof(ack), 0);

        printf("Running...\n");
    }

    printf("Client socket closed.\n");
}

void startServer() {
    // This loop runs by every single client connection (not client interaction and data payloads from clients, for that see handleClient above)
    std::thread([&]() {
        while (serverRunning) {
            // Get the IP Address of the connecting client
            sockaddr_in clientAddr; // Declare the structure to store client address
            int clientAddrLen = sizeof(clientAddr); // Length of client address

            SOCKET clientSocket = accept(g_listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket == INVALID_SOCKET) {
                printf("Connection reset.\n");
                continue; // Continue to accept the next connection
            }

            // Use inet_ntop correctly
            char clientIPStr[INET_ADDRSTRLEN]; // Buffer where the IP string will be stored
            if (inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIPStr, INET_ADDRSTRLEN) == NULL) {
                if (DEBUG) printf("Failed to convert IP address.\n");
                continue;
            }

            std::string ipStr(clientIPStr); // Convert char* to std::string
            std::wstring wideIP(ipStr.begin(), ipStr.end()); // Convert std::string to std::wstring for UI

            std::wstring formattedMessage = L"Received connection from: " + wideIP + L"\r\n";
            wprintf(L"Received connection from: %s\n", wideIP.c_str());
            SendMessage(hStaticServer, WM_SETTEXT, 0, (LPARAM)L"Received new connection..");

            // Send the server signature immediately after accepting the connection
            send(clientSocket, SERVER_SIGNATURE, static_cast<int>(strlen(SERVER_SIGNATURE)), 0);

            std::thread serverThread(handleClient, clientSocket);
            serverThread.detach(); // Detach the thread to handle the client independently
        }
	}).detach();
}

// This function is called in a separate thread to start the server and avoid blocking the main thread
bool initializeServer() {

    SendMessage(hStaticServer, WM_SETTEXT, 0, (LPARAM)L"Starting server...");

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

    // If we reached this point, the server is up and running so we set the flag to true and start the server
    if (DEBUG) printf("Setting atomic serverRunning to true\n");
    serverRunning = true;

    // We start the server in a separate thread to avoid blocking the main thread
    if (DEBUG) printf("Starting server on a separate thread to avoid GUI blocking\n");

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for 1 second so we make sure server is ready to listen
    startServer();

    // Fetch the IP address as a std::wstring
    std::wstring serverIP = getServerIPAddress();

    // Create the message to be sent
    std::wstring message = L"Server started on IP address: " + serverIP;

    // Update the server status in the UI and display the IP address using the IP obtained for serverAddr using inet_ntop like this
    SendMessage(hStaticServer, WM_SETTEXT, 0, (LPARAM)message.c_str());

    // Return true to indicate the server was successfully started
    return true;
}

void serverStartThread() {
    // Start the server in a separate thread to avoid blocking the main thread
    std::thread serverThread(initializeServer);
    serverThread.detach(); // Detach the thread to handle the client independently
}

void shutdownServer() {

    SendMessage(hStaticServer, WM_SETTEXT, 0, (LPARAM)L"Server is shutting down...");

    // Give time for the server to shut down
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for 1 second so the server can shut down
    serverRunning = false; // Set the server running flag to false so it exits the loop and I can clean up and close the server
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for 1 second so the server can shut down

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

bool isServerUp() {
    // Check if the server running flag is true and the listen socket is valid
    return serverRunning.load() && g_listenSocket != INVALID_SOCKET;
}