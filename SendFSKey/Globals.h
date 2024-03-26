#pragma once

#define WM_APPEND_TEXT_TO_CONSOLE (WM_USER + 1)

#include <string>
#include <atomic>

// Atomic variables to control the server and client
extern std::atomic<bool> serverRunning;

// Function to append text to the console
extern void AppendTextToConsole(HWND hStatic, const std::wstring& text); // New version of the function

extern std::wstring mode;
extern std::wstring serverIP;
extern std::wstring serverIPconf;
extern int port;
extern HINSTANCE g_hInst;
extern HWND hStaticServer;
extern HWND hStaticDisplay;

// Operation mode
extern bool isClientMode;

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

// To shutdown or start from anywhere and clean
void cleanupServer();
void shutdownServer();
void startServer(); 
bool isServerUp(); // Used to check if server is running
bool verifyServerSignature(SOCKET serverSocket); // Used to verify server signature

// Client functions
void closeClientConnection();
void cleanupWinsock();