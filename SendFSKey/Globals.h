#pragma once

#include <string>
#include <atomic>

extern std::atomic<bool> serverRunning;
extern std::wstring mode;
extern std::wstring serverIP;
extern int port;
extern HWND hEdit;
extern HINSTANCE g_hInst;

// To use from anywhere
void sendKeyPress(UINT keyCode, bool isKeyDown);
void AppendTextToConsole(HWND, const wchar_t* text);
void MonitorFlightSimulatorProcess();
bool isFlightSimulatorRunning();
std::wstring getServerIPAddress();
std::wstring FormatForDisplay(const std::string& data);

// To help send input to the window in focus from anywhere on my program
UINT SendKeyPressUP(UINT KeyCode);
UINT SendKeyPressDOWN(UINT KeyCode);

// To shutdown or start from anywhere and clean
void shutdownServer();
void startServer(); 
bool ServerStart(); // Used by windowproc in window server start
bool isServerUp(); // Used to check if server is running
bool verifyServerSignature(SOCKET serverSocket); // Used to verify server signature