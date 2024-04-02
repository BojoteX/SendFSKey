#pragma once

#include <mutex>
#include <atomic>
#include <queue>
#include <commctrl.h>
#include "framework.h"
#include "SendFSKey.h"
// #include "utilities.h"
// #include "Logger.h"

#pragma comment(lib, "comctl32.lib")

// For Queue management
extern std::mutex queueMutex;
extern std::queue<std::pair<UINT, bool>> keyEventQueue;

// My Globals
extern bool DEBUG;
extern HANDLE guiReadyEvent;
extern HWND hWndStatusBarClient; 
extern HWND hWndStatusBarServer;
extern HWND hStaticServer;
extern HWND hStaticClient;
extern HINSTANCE g_hInst;
extern HINSTANCE g_hInst_client;
extern HINSTANCE g_hInst_server;
extern bool isClientMode;
extern std::atomic<bool> serverRunning;
extern bool isConnected;
extern std::wstring serverIP;
extern bool queueKeys;
extern int maxQueueSize;
extern NOTIFYICONDATA nid;
extern HANDLE mutexHandle; // The handle for the mutex created in WinMain

// .ini file settings
extern std::wstring mode;
extern std::wstring serverIPconf;
extern std::wstring use_queuing;
extern std::wstring target_window;
extern std::wstring app_process;
extern std::wstring consoleVisibility;
extern std::wstring start_minimized;
extern int port;

// To use from anywhere
// std::wstring getServerIPAddress();

// Key pressing functions
void sendKeyPress(UINT keyCode, bool isKeyDown); // Used by client to send key presses
void getKey(UINT keyCodeNum, bool isSystemKey, bool isKeyDown);
UINT ServerKeyPressDOWN(UINT KeyCode); // Used by server to send key presses
UINT ServerKeyPressUP(UINT KeyCode); // Used by server to send key releases

// Server functions
// void serverStartThread();
// void cleanupServer();
// void shutdownServer();
// void startServer(); 
// bool isServerUp(); // Used to check if server is running
// bool verifyServerSignature(SOCKET serverSocket); // Used to verify server signature

// Client functions
// void closeClientConnection();
// void cleanupWinsock();
// bool establishConnection();
// void clientConnectionThread();