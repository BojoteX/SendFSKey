#pragma once

#include <string>
#include <atomic>

#define WM_APPEND_TEXT_TO_CONSOLE (WM_USER + 1)

// My Globals
extern std::atomic<bool> serverRunning;
extern bool DEBUG;
extern std::wstring mode;
extern std::wstring serverIP;
extern std::wstring serverIPconf;
extern int port;
extern HINSTANCE g_hInst;
extern HWND hStaticServer;
extern HWND hStaticClient;
extern bool isClientMode;

// To use from anywhere
void AppendTextToConsole(HWND hStatic, const std::wstring& text);
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