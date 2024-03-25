#include <Windows.h>
#include <thread>
#include <sstream>
#include "Globals.h"
#include "utilities.h"
#include "NetworkClient.h"
#include "NetworkServer.h"
#include "Resource.h"

HINSTANCE g_hInst = NULL;  // Definition
HWND hEdit = NULL; // Handle to your edit control

LRESULT CALLBACK ServerWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClientWindowProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Operation mode
bool isClientMode;

MSG msg = { 0 };

// App Window Initialization
WNDCLASS wc = { 0 };
WNDCLASS ws = { 0 };

// Initialize windowName and className
wchar_t const* windowName;
wchar_t const* className;

// Our ini file path
std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini"; // Build the full INI file path

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

    g_hInst = hInstance;

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
            std::wstring message = L"Invalid mode entered. Application will exit.";
            MessageBoxW(NULL, message.c_str(), L"Error", MB_ICONERROR);
            return -1; // Exit application due to invalid input
        }
    }
    else {
        wchar_t modeBuffer[256];
        GetPrivateProfileStringW(L"Settings", L"Mode", L"", modeBuffer, 256, iniPath.c_str());
        std::wstring mode = modeBuffer;
        if (mode == L"Client") {
            isClientMode = true;
            // Read other client-specific settings like serverIP
            wchar_t ipBuffer[256];
            GetPrivateProfileStringW(L"Settings", L"IP", L"", ipBuffer, 256, iniPath.c_str());
            serverIP = ipBuffer;
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

    if (isClientMode) {

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"Microsoft Flight Simulator - SendFSKey (Client Mode)";
        wchar_t const* className = L"AceApp";

        // Open our client window
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hInstance = hInstance;
        wc.lpszClassName = className;
        wc.lpfnWndProc = ClientWindowProc;
        wc.lpszMenuName = MAKEINTRESOURCE(IDR_CLIENTMENU);

        if (!RegisterClass(&wc)) return -1;

        HWND hWnd = CreateWindowEx(0, className, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, 800, 600, nullptr, nullptr, hInstance, nullptr);
        if (!hWnd) return -1;

        // Establish connection at startup for client mode
        if (!establishConnection()) {
            MessageBox(NULL, L"Failed to connect to server. Ensure SendFSKey is running on the computer where Flight Simulator is installed.", L"Network Error", MB_ICONERROR | MB_OK);
            // return -1;
        }

        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }
    else if (!isClientMode) {

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"SendFSKey (Server Mode)";
        wchar_t const* className = L"BojoteApp";

        // Open our server window
        ws.hCursor = LoadCursor(nullptr, IDC_ARROW);
        ws.hInstance = hInstance;
        ws.lpszClassName = className;
        ws.lpfnWndProc = ServerWindowProc;
        ws.lpszMenuName = MAKEINTRESOURCE(IDC_SENDFSKEY_SERVER);

        if (!RegisterClass(&ws)) return -1;

        HWND hWndServer = CreateWindowEx(0, className, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 780, 580, NULL, NULL, hInstance, NULL);
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

        if (!initializeServer()) {
            MessageBox(NULL, L"Could not start server. Port is probably busy or server is already running.", L"Network Error", MB_ICONERROR | MB_OK);
            // return -1;  // This return should happen regardless of the DEBUG flag's state
        }

        // This is our window loop
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
    return (int)msg.wParam; // Return the exit code
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
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
    case WM_COMMAND: {
        int wmId = LOWORD(wp);
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDM_ABOUT_BOX), hWnd, AboutDlgProc);
            break;
        case ID_CLIENT_CONNECT:
            if(!establishConnection())
                MessageBox(NULL, L"Failed to connect to server. Ensure SendFSKey is running on the computer where Flight Simulator is installed.", L"Network Error", MB_ICONERROR | MB_OK);
            else
                MessageBox(NULL, L"Connection succesfull", L"Connected", MB_ICONINFORMATION | MB_OK);
            break;
        case ID_CLIENT_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_ENABLE_CONSOLE:
            ToggleConsoleVisibility(L"SendFSKey Client Console");
            break;
        case IDM_RESET_SETTINGS:
            DeleteIniFileAndRestart();
            break;
            // Add cases for other menu items...
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_SYSKEYDOWN: {
        // Extract the scan code from LPARAM
        UINT scanCode = (lp >> 16) & 0x00ff;
        UINT keyCodeNum = static_cast<UINT>(wp);

        keyCodeNum = getKey(keyCodeNum);

        printf("SYSTEM KEY_DOWN: (%lu)\n", keyCodeNum);

        bool is_key_down = 1;
        sendKeyPress(keyCodeNum, is_key_down);

        break;
    }
    case WM_KEYDOWN: {
        // Extract the scan code from LPARAM
        UINT scanCode = (lp >> 16) & 0x00ff;
        UINT keyCodeNum = static_cast<UINT>(wp);

        keyCodeNum = getKey(keyCodeNum);

        printf("KEY_DOWN: (%lu)\n", keyCodeNum);

        bool is_key_down = 1;
        sendKeyPress(keyCodeNum, is_key_down);

        break;
    }
    case WM_SYSKEYUP: {
        // Extract the scan code from LPARAM
        UINT scanCode = (lp >> 16) & 0x00ff;
        UINT keyCodeNum = static_cast<UINT>(wp);

        keyCodeNum = getKey(keyCodeNum);

        printf("SYSTEM KEY_UP: (%lu)\n", keyCodeNum);

        bool is_key_down = 0;
        sendKeyPress(keyCodeNum, is_key_down);

        break;
    }
    case WM_KEYUP:
    {       // Extract the scan code from LPARAM
            UINT scanCode = (lp >> 16) & 0x00ff;
            UINT keyCodeNum = static_cast<UINT>(wp);

            keyCodeNum = getKey(keyCodeNum);

            printf("KEY_UP: (%lu)\n", keyCodeNum);

            bool is_key_down = 0;
            sendKeyPress(keyCodeNum, is_key_down);

            break;
    }
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
    case WM_COMMAND: {
        int wmId = LOWORD(wp); // Move this line inside the WM_COMMAND case
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDM_ABOUT_BOX), hWnd, AboutDlgProc);
            break;
        case IDM_ENABLE_CONSOLE:
            ToggleConsoleVisibility(L"SendFSKey Server Console");
            break;
        case IDM_RESET_SETTINGS:
            DeleteIniFileAndRestart();
            break;
        case ID_SERVER_CONNECT:
            if (!initializeServer())
                MessageBox(NULL, L"Failed to start server. Try restarting this computer and if problem persists contact the author.", L"Network Error", MB_ICONERROR | MB_OK);
            else
                MessageBox(NULL, L"Server started succesfully", L"Connected", MB_ICONINFORMATION | MB_OK);
            break;
        case ID_CLIENT_EXIT:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }
        break; // End of WM_COMMAND
    }
    case WM_DESTROY:
        PostQuitMessage(0); // Signal to end the application
        break;
    default:
        return DefWindowProc(hWnd, msg, wp, lp); // Default message processing
    }
    return 0; // Include a return statement here as good practice
}