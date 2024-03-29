#pragma once

#include <atomic>

// My Globals
extern HANDLE guiReadyEvent;
extern HWND hStaticServer;
extern HWND hStaticClient;
extern HINSTANCE g_hInst;
extern HINSTANCE g_hInst_client;
extern HINSTANCE g_hInst_server;
extern bool isClientMode;
extern bool DEBUG;
extern std::wstring mode;
extern std::wstring serverIP;
extern std::wstring serverIPconf;
extern std::atomic<bool> serverRunning;
extern int port;

// To use from anywhere
void MonitorFlightSimulatorProcess();
bool isFlightSimulatorRunning();
std::wstring getServerIPAddress();
std::wstring FormatForDisplay(const std::string& data);

// Key pressing functions
void sendKeyPress(UINT keyCode, bool isKeyDown); // Used by client to send key presses
void getKey(UINT keyCodeNum, bool isSystemKey, bool isKeyDown);
UINT ServerKeyPressDOWN(UINT KeyCode); // Used by server to send key presses
UINT ServerKeyPressUP(UINT KeyCode); // Used by server to send key releases

// Server functions
void serverStartThread();
void cleanupServer();
void shutdownServer();
void startServer(); 
bool isServerUp(); // Used to check if server is running
bool verifyServerSignature(SOCKET serverSocket); // Used to verify server signature

// Client functions
void closeClientConnection();
void cleanupWinsock();
bool establishConnection();
void clientConnectionThread();