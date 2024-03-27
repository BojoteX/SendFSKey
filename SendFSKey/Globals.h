#pragma once

#include <atomic>

#define WM_APPEND_TEXT (WM_USER + 1)

// My Globals
extern HANDLE guiReadyEvent;
extern std::atomic<bool> serverRunning;
extern bool DEBUG;
extern HWND hStaticServer;
extern HWND hStaticClient;
extern HINSTANCE g_hInst;
extern HINSTANCE g_hInst_client;
extern HINSTANCE g_hInst_server;
extern bool isClientMode;
extern std::wstring mode;
extern std::wstring serverIP;
extern std::wstring serverIPconf;
extern int port;

// To use from anywhere
void AppendTextToConsole(HWND hStc, const std::wstring& text);
void MonitorFlightSimulatorProcess();
bool isFlightSimulatorRunning();
std::wstring getServerIPAddress();
std::wstring FormatForDisplay(const std::string& data);


// Key pressing functions
void sendKeyPress(UINT keyCode, bool isKeyDown); // Used by client to send key presses
void getKey(UINT keyCodeNum, bool isSystemKey, bool isKeyDown);
UINT ServerKeyPressDOWN(UINT KeyCode); // Used by server to send key presses
UINT ServerKeyPressUP(UINT KeyCode); // Used by server to send key releases

// To shutdown or start from anywhere and clean
void cleanupServer();
void shutdownServer();
void startServer(); 
bool isServerUp(); // Used to check if server is running
bool verifyServerSignature(SOCKET serverSocket); // Used to verify server signature

// Client functions
void closeClientConnection();
void cleanupWinsock();
