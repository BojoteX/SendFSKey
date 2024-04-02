#pragma once

#include <mutex>
#include <atomic>
#include <queue>
#include <commctrl.h>
#include <TlHelp32.h>
#include "framework.h"
#include "SendFSKey.h"

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
extern bool has_permission;
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
