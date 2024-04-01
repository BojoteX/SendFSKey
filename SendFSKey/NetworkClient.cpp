#include <thread>
#include <ws2tcpip.h>
#include "NetworkClient.h"

// Define the expected server signature
const char* EXPECTED_SERVER_SIGNATURE = "SendFSKeySSv1";

// Global persistent socket
SOCKET g_persistentSocket = INVALID_SOCKET; // Global persistent socket

// Queue management
std::mutex queueMutex;
std::queue<std::pair<UINT, bool>> keyEventQueue;

// Reconnection flag
std::atomic<bool> isReconnecting(false);
bool isConnected = false;

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
    tv.tv_sec = 1;  // 1 seconds
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
            std::wstring message_status_bar = L"Connected to " + serverIPconf;

            // Update the server status in the UI and display the IP address using the IP obtained for serverAddr using inet_ntop like this
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)message.c_str());
            SendMessage(hWndStatusBarClient, SB_SETTEXT, 1 | SBT_POPOUT, (LPARAM)message_status_bar.c_str());

            return true; // Connection and verification successful
        }
        else {
            printf("Connection failed with error: %d\n", so_error);
            // Update the server status in the UI and display the IP address using the IP obtained for serverAddr using inet_ntop like this
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Connection failed with error " + so_error);
        }
    }
    else {
        printf("Connection timed out, failed or server is not running.\n");
        SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Connection timed out or failed");

        if(!isReconnecting and start_minimized == L"No")
            MessageBox(NULL, L"Failed to connect to server. Check SendFSKey is running on the remote computer in 'Server Mode'", L"Network Error", MB_ICONERROR | MB_OK);
    }

    SendMessage(hWndStatusBarClient, SB_SETTEXT, 1 | SBT_POPOUT, (LPARAM)L"Not Connected");

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

    // If we're reconnecting and not queueing, ignore the input
    if (isReconnecting and !queueKeys) {
        printf("\n");
        printf("*****************************************************************\n");
        printf("* [ATTENTION] Virtual-Key Code (%u) ignored while re-connecting *\n", keyCode);
        printf("*****************************************************************\n\n");
        return;
    }

    // We'll try to reconnect infinitely until we're successful as the reconnection process is handled in a separate thread and barely uses any resources

    std::wstring sent_message = L"Virtual-Key Code sent: (" + std::to_wstring(keyCode) + L")";

    unsigned char buffer[5];
    buffer[0] = isKeyDown ? 'D' : 'U'; // Event type flag
    memcpy(buffer + 1, &keyCode, sizeof(keyCode)); // Copy keyCode into buffer

    auto reconnectAndResend = []() {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Initial pause before attempting to reconnect

        if (!isReconnecting.exchange(true)) {
            SendMessage(hWndStatusBarClient, SB_SETTEXT, 1 | SBT_POPOUT, (LPARAM)L"Reconnecting...");
            printf("Connection error. Attempting to reconnect...\n");
            closeClientConnection(); // Close the existing connection

            bool isConnected = false;
            while (!isConnected) {
                Sleep(5000); // Wait for 5 seconds before retrying

                isConnected = establishConnection();
                if (!isConnected) {
                    printf("Reconnect attempt failed. Waiting 5 seconds before retrying...\n");
                }
            }

            printf("Reconnection successful.\n");
            SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)L"Reconnection successful.");
            isReconnecting = false; // Reset the reconnection flag

            if (!queueKeys) {
                std::lock_guard<std::mutex> lock(queueMutex);
                // Clear the queue if queueKeys is false
                while (!keyEventQueue.empty()) keyEventQueue.pop();
                return; // Exit the function since we're not processing the queue
            }

            // If queueKeys is true, continue to process the queue
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!keyEventQueue.empty()) {
                auto [key, isDown] = keyEventQueue.front();
                keyEventQueue.pop();

                unsigned char resendBuffer[5];
                resendBuffer[0] = isDown ? 'D' : 'U';
                memcpy(resendBuffer + 1, &key, sizeof(key));

                if (send(g_persistentSocket, reinterpret_cast<char*>(resendBuffer), sizeof(resendBuffer), 0) != SOCKET_ERROR) {
                    if (isDown)
                        printf("[KEY_DOWN] for Virtual-Key Code %u successfully resent.\n", key);
                    else
                        printf("[KEY_UP] for Virtual-Key Code %u successfully resent.\n", key);
                }
                else {
                    if (isDown)
                        printf("[KEY_DOWN] for Virtual-Key Code %u could NOT be sent.\n", key);
                    else
                        printf("[KEY_UP] for Virtual-Key Code %u could NOT be sent.\n", key);
                    // Consider re-queueing the event or handling failure
                }
            }
        }
        };

    if (send(g_persistentSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0) == SOCKET_ERROR) {
        if (queueKeys) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (keyEventQueue.size() < maxQueueSize) { // Check if queue is not full
                keyEventQueue.emplace(keyCode, isKeyDown);
            }
            else {
                // Optionally log or handle the case when the queue is full and the keypress is ignored
                printf("Queue is full. Ignoring key press: %u %s\n", keyCode, isKeyDown ? "DOWN" : "UP");
            }
        }

        std::thread(reconnectAndResend).detach(); // Attempt reconnection in a separate thread
    }
    else {
        // If send was successful, wait for server acknowledgment
        char ack;
        if (recv(g_persistentSocket, &ack, sizeof(ack), 0) > 0) { // Blocking call until ack is received
            if (isKeyDown)
                SendMessage(hStaticClient, WM_SETTEXT, 0, (LPARAM)sent_message.c_str());
        }
    }
}

// Cleanup Winsock
void cleanupWinsock() {
    WSACleanup();
}