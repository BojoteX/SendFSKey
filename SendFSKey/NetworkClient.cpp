#include <thread>
#include <ws2tcpip.h>
#include "NetworkClient.h"

// Define the expected server signature
const char* EXPECTED_SERVER_SIGNATURE = "SendFSKeySSv1";

SOCKET g_persistentSocket = INVALID_SOCKET; // Global persistent socket

// Initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error initializing winsock.\n");
        return false;
    }
    return true;
}

bool verifyServerSignature(SOCKET serverSocket) {
    char receivedSignature[256];
    int bytesReceived = recv(serverSocket, receivedSignature, sizeof(receivedSignature) - 1, 0);
    if (bytesReceived > 0) {
        receivedSignature[bytesReceived] = '\0'; // Null-terminate the received string
        if (strcmp(receivedSignature, EXPECTED_SERVER_SIGNATURE) == 0) {
            if (DEBUG) printf("Signature match.\n");
            return true; // Signature matches
        }
        else {
            closesocket(serverSocket); // Signature does not match, close the connection
            WSACleanup();
            if (DEBUG) printf("Incorrect signature.\n");
            return false;
        }
    }
    closesocket(serverSocket); // If no data received, close the connection
    WSACleanup();
    printf("Closing connection and doing cleanup.\n");
    return false;
}

bool establishConnection() {
    // Initiate the connection attempt in a new thread 
    if (g_persistentSocket != INVALID_SOCKET) {
        printf("Connection is already established.\n");
        return true; // Connection is already established
    }

    std::thread([]() {
        // Initiate the connection attempt in a new thread only if no active socket was found.
        if (establishConnectionAsync()) {
            // Connection successful
            printf("Connection established.\n");
            // Update GUI to show connection successful, ensuring to do so in a thread-safe manner.
            std::wstring finalMessage = L"Connection successfully established or already connected.\r\n";
        }
        else {
            // Connection failed
            printf("Failed to establish connection.\n");
            // Update GUI to show connection failed, ensuring to do so in a thread-safe manner.
            std::wstring finalMessage = L"Failed to establish connection.\n";
        }
    }).detach(); // Detach the thread to allow it to run independently

    // Will always return true since the connection attempt is in a separate thread, so don't check the return value here but rather send a message to the GUI
    // We still return true here to indicate that the connection attempt has been initiated but not necessarily completed yet.
    return false;
}

bool establishConnectionAsync() {
    if (g_persistentSocket != INVALID_SOCKET) {
        printf("The connection is already established.\n");
        return true; // Connection is already established
    }

    g_persistentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_persistentSocket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        return false;
    }

    // Set the socket to non-blocking mode for the connection attempt
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(g_persistentSocket, FIONBIO, &mode);

    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress)); // Ensure the structure is empty
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    InetPton(AF_INET, serverIPconf.c_str(), &serverAddress.sin_addr); // Assuming serverIP.conf is std::string

    // Attempt to connect (non-blocking)
    int connectResult = connect(g_persistentSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (connectResult == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            printf("Connect failed immediately with error: %d\n", error);
            closesocket(g_persistentSocket);
            g_persistentSocket = INVALID_SOCKET;
            return false;
        }
    }

    // Setup select parameters for checking writeability (connection success)
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(g_persistentSocket, &writefds);

    // Setup timeval struct for timeout
    timeval tv;
    tv.tv_sec = 3;  // 3 seconds
    tv.tv_usec = 0;

    // Check if the socket is writable (connection succeeded) within the timeout
    if (select(0, NULL, &writefds, NULL, &tv) > 0) {
        // Check if there were any socket errors
        int so_error;
        socklen_t len = sizeof(so_error);
        getsockopt(g_persistentSocket, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);

        if (so_error == 0) {
            // Connection successful

            // Set socket back to blocking mode
            mode = 0;
            ioctlsocket(g_persistentSocket, FIONBIO, &mode);

            // Verify server signature
            if (!verifyServerSignature(g_persistentSocket)) {
                if (DEBUG) printf("Could not verify server signature.\n");
                closesocket(g_persistentSocket);
                g_persistentSocket = INVALID_SOCKET;
                return false;
            }

            AppendTextToConsole(hStaticClient, L"Connected");
            return true; // Connection and verification successful
        }
        else {
            printf("Connection failed with error: %d\n", so_error);
        }
    }
    else {
        printf("Connection timed out or failed.\n");
    }

    // Cleanup on failure
    closesocket(g_persistentSocket);
    g_persistentSocket = INVALID_SOCKET;
    return false;
}

// Close the persistent connection
void closeClientConnection() {
    if (g_persistentSocket != INVALID_SOCKET) {
        closesocket(g_persistentSocket);
        g_persistentSocket = INVALID_SOCKET;
    }
}

void sendKeyPress(UINT keyCode, bool isKeyDown) {
    unsigned char buffer[5];
    buffer[0] = isKeyDown ? 'D' : 'U'; // Event type flag

    // Ensure little-endian byte order for the keyCode
    memcpy(buffer + 1, &keyCode, sizeof(keyCode));

    // First attempt to send the key event
    if (send(g_persistentSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0) == SOCKET_ERROR) {

        printf("Connection error. Attempting to reconnect...\n");

        // Close the old connection
        closeClientConnection(); // Assume this resets g_persistentSocket and handles necessary cleanup

        // Try to re-establish the connection with exponential backoff
        bool isConnected = false;
        for (int attempts = 0; attempts < 5 && !isConnected; attempts++) {
            Sleep((1 << attempts) * 1000); // Exponential backoff

            isConnected = establishConnection(); // Tries to reconnect and updates g_persistentSocket on success
            if (!isConnected) {
                printf("Reconnect attempt %d failed. Waiting %d seconds before retrying...\n", attempts + 1, (1 << attempts));
            }
        }

        // Check if reconnection was successful
        if (isConnected) {
            printf("Reconnection successful. Resending data...\n");
            // After reconnecting, try sending the signature again

            if (send(g_persistentSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0) == SOCKET_ERROR) {
                printf("Failed to send data after reconnecting.\n");
                // Handle failure to resend data here
            }
            else {
                char ack;
                recv(g_persistentSocket, &ack, sizeof(ack), 0); // Await acknowledgment
            }
        }
        else {
            MessageBox(NULL, L"Failed to reconnect to server after multiple attempts. Exiting.", L"Network Error", MB_ICONERROR | MB_OK);

            cleanupWinsock();
            printf("Closing connection and doing cleanup before exiting.\n");
            ExitProcess(1); // Terminate application if unable to reconnect after max attempts
        }
    }
    else {
        // If send was successful, wait for server acknowledgment
        char ack;
        recv(g_persistentSocket, &ack, sizeof(ack), 0); // Blocking call until ack is received
    }
}

// Cleanup Winsock
void cleanupWinsock() {
    WSACleanup();
}