#include <windows.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <thread>
#include <string>
#include <shlwapi.h>
#include "Globals.h"
#include "NetworkClient.h"
#include "NetworkServer.h"
#include "Resource.h"

#pragma comment(lib, "shlwapi.lib")

LRESULT CALLBACK ServerWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClientWindowProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Debug Mode
bool DEBUG = 1; // Debug console

// Operation mode
bool isClientMode;

MSG msg = { 0 };

// App Window Initialization
WNDCLASS wc = { 0 };
WNDCLASS ws = { 0 };

// Settings
std::wstring mode;
std::wstring serverIP;
int port = 8028;

// Initialize windowName and className
wchar_t const* windowName;
wchar_t const* className;

std::wofstream logFile;

void OpenLogFile() {
    logFile.open("SendFSKey.log", std::wofstream::out | std::wofstream::app); // Open for writing in append mode
    if (!logFile.is_open()) {
        std::wcerr << L"Failed to open log file." << std::endl;
    }
}

void Log(const std::wstring& message) {
    if (logFile.is_open()) {
        logFile << message << std::endl;
        // For immediate writing, you can flush after each log entry
        logFile.flush();
    }
}

void CloseLogFile() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

// Function to check if the INI file exists
bool IniFileExists(const std::string& filename) {
    std::ifstream ifile(filename.c_str());
    return ifile.good();
}

// Utility function to get the directory of the current executable
std::wstring GetExecutableDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH); // Get the full executable path
    PathRemoveFileSpecW(path); // Remove the executable name, leaving the directory path
    return std::wstring(path);
}

void WriteSettingsToIniFile(const std::wstring& mode, const std::wstring& ip) {
    std::wstring iniPath = GetExecutableDir() + L"\\SendFSKey.ini"; // Build the full INI file path

    WritePrivateProfileStringW(L"Settings", L"Mode", mode.c_str(), iniPath.c_str());
    if (mode == L"Client") {
        WritePrivateProfileStringW(L"Settings", L"IP", ip.c_str(), iniPath.c_str());
    }
    WritePrivateProfileStringW(L"Settings", L"Port", std::to_wstring(port).c_str(), iniPath.c_str());
}

void AppendTextToConsole(HWND hEdit, const wchar_t* text) {
    // Calculate the new end of the text buffer so we can set the selection
    // to the end of the text and ensure the newly appended text is visible.
    int idxEnd = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, (WPARAM)idxEnd, (LPARAM)idxEnd);

    // Append the text by replacing the current selection (which is nothing at the end of the text)
    SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)text);

    // Scroll to the end of the text
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

    OpenLogFile();
    Log(L"Application Start");

    // Check if the INI file exists
    if (!IniFileExists("SendFSKey.ini")) {
        // INI file doesn't exist, prompt user for mode

        if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, SettingsDialogProc) == IDC_OK) {
            // The user clicked OK and the settings have been captured.
            WriteSettingsToIniFile(mode, serverIP); // Use the global `mode` variable set in the dialog procedure
        }
        else {
            // The user clicked Cancel or closed the dialog
            return -1; // Exit the application as no settings have been configured
        }

        // Now, use `mode` to determine the client or server operation
        if (mode == L"Server") {
            isClientMode = false;
        }
        else if (mode == L"Client") {
            isClientMode = true;
        }
        else {
            MessageBoxA(NULL, "Invalid mode entered. Application will exit.", "Error", MB_ICONERROR);
            return -1; // Exit application due to invalid input
        }
    }
    else {
        std::wstring iniPath = GetExecutableDir() + L"\\SendFSKey.ini";

        wchar_t modeBuffer[256];
        GetPrivateProfileStringW(L"Settings", L"Mode", L"", modeBuffer, 256, iniPath.c_str());
        std::wstring mode = modeBuffer;

        if (mode == L"Client") {
            isClientMode = true;
            // Read other client-specific settings like serverIP
            wchar_t ipBuffer[256];
            GetPrivateProfileStringW(L"Settings", L"IP", L"", ipBuffer, 256, iniPath.c_str());
            serverIP = ipBuffer;
            // Read port if necessary
        }
        else if (mode == L"Server") {
            isClientMode = false;
            // Server-specific initializations
        }
        else {
            MessageBox(NULL, L"Selected mode has not been implemented.", L"Error", MB_ICONERROR | MB_OK);
        }
    }

    // Initialize Winsock for both client and server
    if (!initializeWinsock()) {
        return -1; // Exit if Winsock initialization fails
    }

    if (DEBUG) {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    }

    if (isClientMode) {
        SetConsoleTitleW(L"SendFSKey Client");
        // Establish connection at startup for client mode
        if (!establishConnection()) {
            printf("Could not connect to IP Address %ls using port %d\n", serverIP.c_str(), port);
            MessageBox(NULL, L"Failed to connect to server. Will exit now.", L"Network Error", MB_ICONERROR | MB_OK);
            return -1;
        }

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"Microsoft Flight Simulator - SendFSKey (Client Mode)";
        wchar_t const* className = L"AceApp";

        // Open our client window
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hInstance = hInstance;
        wc.lpszClassName = className;
        wc.lpfnWndProc = ClientWindowProc;

        if (!RegisterClass(&wc)) return -1;

        HWND hWnd = CreateWindowEx(0, className, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, 800, 600, nullptr, nullptr, hInstance, nullptr);
        if (!hWnd) return -1;

        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }
    else if (!isClientMode) {
        SetConsoleTitleW(L"SendFSKey Server");
        if (!initializeServer()) {
            MessageBox(NULL, L"Could not start server. Port is probably busy or not enough permissions.", L"Network Error", MB_ICONERROR | MB_OK);
            return -1;  // This return should happen regardless of the DEBUG flag's state
        }

        std::wstring serverIP = getServerIPAddress();
        printf("SendFSKey v1.0\n");
        printf("Copyright(c) 2024 by Jesus \"Bojote\" Altuve\n");
        printf("\n");
        printf("Server started succesfully on IP Address %ls using port %d\n", serverIP.c_str(), port);
        printf("Ready to accept connections. Run SendFSKey on remote computer in client mode\n");

        std::thread clientThread(startServer);
        clientThread.detach(); // Detach the thread to handle the client independently

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"SendFSKey (Server Mode)";
        wchar_t const* className = L"BojoteApp";

        // Open our server window
        ws.hCursor = LoadCursor(nullptr, IDC_ARROW);
        ws.hInstance = hInstance;
        ws.lpszClassName = className;
        ws.lpfnWndProc = ServerWindowProc;

        if (!RegisterClass(&ws)) return -1;

        HWND hWndServer = CreateWindowEx(0, className, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
        HWND hEdit;
        if (hWndServer) {
            hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 10, 10, 780, 580, hWndServer, (HMENU)IDC_MAIN_EDIT, GetModuleHandle(NULL), NULL);
            if (hEdit) {


                HFONT hFont = CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
                SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

                // Scroll to the bottom
                SendMessage(hEdit, EM_SCROLLCARET, 0, 0);

                AppendTextToConsole(hEdit, L"Initializing server...\n");
                AppendTextToConsole(hEdit, L"Server initialized successfully!\n");

            }
            else {
                // Handle edit control creation failure
            }
        }
        else {
            // Handle window creation failure
        }

        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }
    else {
        MessageBoxA(NULL, "Mode not set. Application will now exit.", "Error", MB_ICONERROR);
        return -1; // Exit application due to invalid input or incorrect ini file
    }

    // Cleanup before exit
    if (isClientMode) {
        closeClientConnection(); // For client
    }
    else {
        // For server mode, insert cleanup operations here
        cleanupServer();
    }
    cleanupWinsock();

    CloseLogFile(); // Ensure this is the last thing you do before exiting WinMain
    return (int)msg.wParam; // Return the exit code
}

// Implement the dialog procedure function
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        // Set the default values or configurations for your dialog controls here
        HWND hwndCombo = GetDlgItem(hDlg, IDC_COMBOBOX); // Replace with your combo box ID
        SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Client"); // Use wide string literals
        SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Server"); // Use wide string literals
        SendMessage(hwndCombo, CB_SETCURSEL, 0, 0); // Select "Client" by default
        return TRUE; // Return TRUE to set the keyboard focus to the control specified by wParam.
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDC_OK: {
            // Change the buffer to a wide string buffer.
            wchar_t modeBuffer[7] = { 0 };
            GetDlgItemText(hDlg, IDC_COMBOBOX, modeBuffer, _countof(modeBuffer)); // Use the wide-char version of GetDlgItemText
            mode = modeBuffer; // Now modeBuffer is a wide string, and this should work.

            if (mode == L"Client") { // Note the 'L' prefix to create a wide string literal.
                wchar_t ipBuffer[16] = { 0 };
                GetDlgItemText(hDlg, IDC_IPADDRESS, ipBuffer, _countof(ipBuffer)); // Again, make sure this is the wide-char version.
                serverIP = ipBuffer; // This is fine as both are now wide strings.
            }
            WriteSettingsToIniFile(mode, serverIP);
            EndDialog(hDlg, IDC_OK);
            return TRUE;
        }
        case IDC_CANCEL: {
            EndDialog(hDlg, IDC_CANCEL); // Close the dialog box with IDCANCEL result
            return TRUE;
        }
        }
        break;
    }
    case WM_CLOSE: {
        EndDialog(hDlg, IDC_CANCEL); // User clicked the close button on the title bar
        return TRUE;
    }
    }
    return FALSE; // Return FALSE if you don't process the message
}

LRESULT CALLBACK ClientWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_KEYDOWN:
        if (isClientMode) {
            // Send key press in client mode
            sendKeyPress(static_cast<UINT>(wp));
            if (DEBUG) {
                wchar_t charCode = toupper(static_cast<wchar_t>(wp));
                wprintf(L"Key Down: %c ", charCode);
                printf("(%llu)\n", static_cast<unsigned long long>(wp));
            }
        }
        break;
    case WM_KEYUP:
        if (isClientMode) {
            if (DEBUG) {
                wchar_t charCode = toupper(static_cast<wchar_t>(wp));
                wprintf(L"Key Up: %c ", charCode);
                printf("(%llu)\n", static_cast<unsigned long long>(wp));
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}

LRESULT CALLBACK ServerWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0); // Signal to end the application
        break; // Return 0 to indicate message has been handled
    default:
        return DefWindowProc(hWnd, msg, wp, lp); // Default message processing
    }
    return 0; // Include a return statement here as good practice (though typically not reached)
}