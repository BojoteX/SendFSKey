#include <ws2tcpip.h>
#include "NetworkClient.h"

SOCKET g_persistentSocket = INVALID_SOCKET; // Global persistent socket

// Initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    return true;
}

bool establishConnection() {
    if (g_persistentSocket != INVALID_SOCKET) return true; // Connection is already established

    g_persistentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_persistentSocket == INVALID_SOCKET) {
        return false;
    }

    // Set the socket to non-blocking mode for the connection attempt
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(g_persistentSocket, FIONBIO, &mode);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    InetPtonW(AF_INET, serverIP.c_str(), &serverAddress.sin_addr);

    // Attempt to connect (non-blocking)
    connect(g_persistentSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));

    // Setup timeval struct for timeout
    timeval tv;
    tv.tv_sec = 3;  // 3 seconds
    tv.tv_usec = 0;

    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(g_persistentSocket, &writefds);

    // Check if the socket is writable (connection succeeded) within the timeout
    if (select(0, NULL, &writefds, NULL, &tv) > 0) {
        // Check if there were any socket errors
        int so_error;
        socklen_t len = sizeof(so_error);
        getsockopt(g_persistentSocket, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);

        if (so_error == 0) {
            // Connection successful
            mode = 0;  // Set socket back to blocking mode
            ioctlsocket(g_persistentSocket, FIONBIO, &mode);
            return true;
        }
    }

    // If we reach here, the connection failed or timed out
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
            // After reconnecting, try sending the key code again
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