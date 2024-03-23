#include <ws2tcpip.h>
#include "Globals.h"
#include "NetworkClient.h"

SOCKET g_persistentSocket = INVALID_SOCKET; // Global persistent socket

// Initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return false;
    }
    return true;
}

bool establishConnection() {
    if (g_persistentSocket != INVALID_SOCKET) return true; // Connection is already established

    g_persistentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_persistentSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
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

void sendKeyPress(UINT keyCode) {
    // Convert the numerical value of the key code to a string
    char buffer[16];
    int len = snprintf(buffer, sizeof(buffer), "%u", keyCode);

    // Try to send the key code over the network
    if (len > 0 && send(g_persistentSocket, buffer, len, 0) == SOCKET_ERROR) {
        printf("Connection error. Attempting to reconnect...\n");

        // Close the old connection
        closeClientConnection();

        // Try to re-establish the connection with exponential backoff
        int attempts = 0;
        const int max_attempts = 5;
        while (!establishConnection() && attempts < max_attempts) {
            // Wait before retrying (exponential backoff)
            int wait_time = 1 << attempts; // Equivalent to 2^attempts, using bitwise shift for integer exponentiation
            printf("Reconnect attempt failed. Waiting %d seconds before retrying...\n", wait_time);
            Sleep(wait_time * 1000); // Sleep uses milliseconds
            attempts++;
        }

        // After reconnecting, try sending the key code again
        if (attempts < max_attempts && send(g_persistentSocket, buffer, len, 0) != SOCKET_ERROR) {
            // Wait for server acknowledgment
            char ack;
            recv(g_persistentSocket, &ack, sizeof(ack), 0);  // This will block until ack is received
        }
        else {
            // Failed to reconnect or send data after retries
            MessageBox(NULL, L"Failed to reconnect to server. Will exit now.", L"Network Error", MB_ICONERROR | MB_OK);
            cleanupWinsock();
            ExitProcess(1);
        }
    }
    else {
        // If send was successful, wait for server acknowledgment
        char ack;
        recv(g_persistentSocket, &ack, sizeof(ack), 0);  // This will block until ack is received
    }
}

// Cleanup Winsock
void cleanupWinsock() {
    WSACleanup();
}