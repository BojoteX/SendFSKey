#include <Windows.h>
#include <thread>
#include <sstream>
#include "Globals.h"
#include "utilities.h"
#include "NetworkClient.h"
#include "NetworkServer.h"
#include "Resource.h"

LRESULT CALLBACK ServerWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClientWindowProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HWND hEdit = NULL; // Handle to your edit control

// Operation mode
bool isClientMode;
bool is_key_down;

MSG msg = { 0 };

// App Window Initialization
WNDCLASS wc = { 0 };
WNDCLASS ws = { 0 };

// Initialize windowName and className
wchar_t const* windowName;
wchar_t const* className;

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

    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
  
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

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"SendFSKey (Server Mode)";
        wchar_t const* className = L"BojoteApp";

        // Open our server window
        ws.hCursor = LoadCursor(nullptr, IDC_ARROW);
        ws.hInstance = hInstance;
        ws.lpszClassName = className;
        ws.lpfnWndProc = ServerWindowProc;

        if (!RegisterClass(&ws)) return -1;

        HWND hWndServer = CreateWindowEx(0, className, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 200, NULL, NULL, hInstance, NULL);
        if (hWndServer) {
            hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 10, 10, 780, 580, hWndServer, (HMENU)IDC_MAIN_EDIT, GetModuleHandle(NULL), NULL);
            if (hEdit) {


                HFONT hFont = CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
                SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

                // Scroll to the bottom
                SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
            }
            else {
                // Handle edit control creation failure
            }
        }
        else {
            // Handle window creation failure
        }

        std::wstringstream ws;
        std::wstring serverIP = getServerIPAddress();

        // Concatenating strings and variables

        ws << L"SendFSKey v1.0 - Copyright(c) 2024 by Jesus \"Bojote\" Altuve\r\n";
        ws << L"\r\n";
        ws << L"Server started successfully on ";
        ws << L"IP Address: " << serverIP.c_str() << L", "; // Correct usage of .c_str()
        ws << L"using port: " << port << L".\r\n";
        ws << L"Ready to accept connections. Just run SendFSKey on remote computer in client mode\r\n";
        ws << L"\r\n";

        // Converting wstringstream to wstring
        std::wstring finalMessage = ws.str();
        
        AppendTextToConsole(hEdit, finalMessage.c_str());

        // Just before starting the UI look we spawnn our thread and detach it
        std::thread ServerThread(startServer);
        ServerThread.detach(); // Detach the thread to handle the client independently

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

                is_key_down = 1;
                // Send key press in client mode
                sendKeyPress(static_cast<UINT>(wp), is_key_down);

                wchar_t charCode = toupper(static_cast<wchar_t>(wp));
                wprintf(L"Key Down: %c ", charCode);
                printf("(%llu)\n", static_cast<unsigned long long>(wp));

        }
        break;
    case WM_KEYUP:
        if (isClientMode) {

                is_key_down = 0;
                // Send key press in client mode
                sendKeyPress(static_cast<UINT>(wp), is_key_down);

                wchar_t charCode = toupper(static_cast<wchar_t>(wp));
                wprintf(L"Key Up: %c ", charCode);
                printf("(%llu)\n", static_cast<unsigned long long>(wp));

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