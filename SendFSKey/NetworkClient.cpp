#include <mutex> 
#include <thread>
#include <ws2tcpip.h>
#include "NetworkClient.h"

// Define the expected server signature
const char* EXPECTED_SERVER_SIGNATURE = "SendFSKeySSv1";

SOCKET g_persistentSocket = INVALID_SOCKET; // Global persistent socket

std::atomic<bool> isReconnecting(false);

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

void clientConnectionThread() {
    // Establish the connection
    std::thread clientThread(establishConnection);  // pass false when not retrying connection so that it shows the MessageBox
    clientThread.detach(); // Detach the thread to handle the client independently
}

bool establishConnection() {

    // Create the message to be sent
    std::wstring message = L"Trying to connect to " + serverIPconf;

    // Update the server status in the UI and display the IP address using the IP obtained for serverAddr using inet_ntop like this
    SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)message.c_str());

    // Always close the existing connection before attempting to establish a new one
    closeClientConnection();

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

            // Create the message to be sent
            std::wstring message = L"Connected succesfully to " + serverIPconf;

            // Update the server status in the UI and display the IP address using the IP obtained for serverAddr using inet_ntop like this
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)message.c_str());

            return true; // Connection and verification successful
        }
        else {
            printf("Connection failed with error: %d\n", so_error);
            // Update the server status in the UI and display the IP address using the IP obtained for serverAddr using inet_ntop like this
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Connection failed with error " + so_error);
        }
    }
    else {
        printf("Connection timed out or failed.\n");
        SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Connection timed out or failed");

        if(!isReconnecting)
            MessageBox(NULL, L"Failed to connect to server.", L"Network Error", MB_ICONERROR | MB_OK);
    }

    // Cleanup on failure
    closesocket(g_persistentSocket);
    g_persistentSocket = INVALID_SOCKET;
    return false;
}

// Close the persistent connection
void closeClientConnection() {
    if (g_persistentSocket != INVALID_SOCKET) {
        SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Closing connection.");
        closesocket(g_persistentSocket);
        g_persistentSocket = INVALID_SOCKET;
    }

}

void sendKeyPress(UINT keyCode, bool isKeyDown) {
    std::wstring sent_message = L"Virtual-Key Code sent: (" + std::to_wstring(keyCode) + L")";

    unsigned char buffer[5];
    buffer[0] = isKeyDown ? 'D' : 'U'; // Event type flag

    // Ensure little-endian byte order for the keyCode
    memcpy(buffer + 1, &keyCode, sizeof(keyCode));

    auto reconnectAndResend = [buffer, isKeyDown, sent_message]() mutable {

        std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause before attempting to reconnect

        if (!isReconnecting.exchange(true)) {
            printf("Connection error. Attempting to reconnect...\n");
            closeClientConnection(); // Close the old connection

            bool isConnected = false;
            while (!isConnected) {
                Sleep(5000); // Wait for 5 seconds before retrying

                isConnected = establishConnection();
                if (!isConnected) {
                    printf("Reconnect attempt failed. Waiting 5 seconds before retrying...\n");
                }
            }

            // Clear the buffer before resending to avoid sending old data or garbage
            // memset(buffer, 0, sizeof(buffer));

            printf("Reconnection successful. Resending data...\n");
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Reconnection successful. Resending data...");

            if (send(g_persistentSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0) == SOCKET_ERROR) {
                printf("Failed to send data after reconnecting.\n");
                SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Failed to send data after reconnecting.");
            }
            else {
                char ack;
                recv(g_persistentSocket, &ack, sizeof(ack), 0); // Await acknowledgment
                if (isKeyDown) {
                    SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)sent_message.c_str());
                }
            }

            isReconnecting = false; // Reset the flag once reconnection is complete
        }
    };

    // First attempt to send the key event
    if (send(g_persistentSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0) == SOCKET_ERROR) {

        // Use threading but can't read keyup events
        std::thread(reconnectAndResend).detach(); // Handle reconnection in a detached thread

        // Or Syncronously (which solves the re-sending issue)
        // reconnectAndResend();
    }
    else {
        // If send was successful, wait for server acknowledgment
        char ack;
        recv(g_persistentSocket, &ack, sizeof(ack), 0); // Blocking call until ack is received

        // Message sent
        if (isKeyDown)
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)sent_message.c_str());
    }
}

void sendKeyPressOLD(UINT keyCode, bool isKeyDown) {
    std::wstring sent_message = L"Virtual-Key Code sent: (" + std::to_wstring(keyCode) + L")";

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

            isConnected = establishConnection(); // pass true when retrying connection so that it doesn't show the MessageBox
            if (!isConnected) {
                printf("Reconnect attempt %d failed. Waiting %d seconds before retrying...\n", attempts + 1, (1 << attempts));
            }
        }

        // Check if reconnection was successful
        if (isConnected) {
            printf("Reconnection successful. Resending data...\n");
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Reconnection successful. Resending data...");

            // After reconnecting, try sending the signature again

            if (send(g_persistentSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0) == SOCKET_ERROR) {
                printf("Failed to send data after reconnecting.\n");
                SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Failed to send data after reconnecting.");
                // Handle failure to resend data here
            }
            else {
                char ack;
                recv(g_persistentSocket, &ack, sizeof(ack), 0); // Await acknowledgment

                // Message sent

                if (isKeyDown)
                    SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)sent_message.c_str());
            }
        }
        else {
            MessageBox(NULL, L"Failed to reconnect to server after multiple attempts. Exiting.", L"Network Error", MB_ICONERROR | MB_OK);
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Failed to reconnect to server.");

            closeClientConnection();
            cleanupWinsock();
            printf("Closing connection and doing cleanup before exiting.\n");
            ExitProcess(1); // Terminate application if unable to reconnect after max attempts
        }
    }
    else {
        // If send was successful, wait for server acknowledgment
        char ack;
        recv(g_persistentSocket, &ack, sizeof(ack), 0); // Blocking call until ack is received

        // Message sent
        if(isKeyDown)
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)sent_message.c_str());
    }
}

// Cleanup Winsock
void cleanupWinsock() {
    WSACleanup();
}