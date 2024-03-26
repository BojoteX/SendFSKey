#include <Windows.h>
#include <thread>
#include <sstream>
#include "Globals.h"
#include "utilities.h"
#include "NetworkClient.h"
#include "NetworkServer.h"
#include "Resource.h"

HINSTANCE g_hInst = NULL;  // Definition
HWND hStaticServer = NULL; // Handle to your server edit control
HWND hStaticDisplay = NULL; // Handle to your client edit control

int DEFAULT_WIDTH = 560;
int DEFAULT_HEIGHT = 155;
int FONT_SIZE = 18;

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
            WriteSettingsToIniFile(mode, serverIPconf); // Use the global `mode` variable set in the dialog procedure
        }
        else {
            // The user clicked Cancel or closed the dialog
            MessageBox(NULL, L"Operation was cancelled.", L"CANCEL", MB_ICONERROR | MB_OK);
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
            // Read other client-specific settings like serverIPconf
            wchar_t ipBuffer[256];
            GetPrivateProfileStringW(L"Settings", L"IP", L"", ipBuffer, 256, iniPath.c_str());
            serverIPconf = ipBuffer;
        }
        else if (mode == L"Server") {
            isClientMode = false;
            // Server-specific initializations
        }
        else {
            MessageBox(NULL, L"Selected mode has not been implemented.", L"Error", MB_ICONERROR | MB_OK);
        }
    }

    // Show the console window, all messages before console is open display MessageBox. We can later conditionally check our ini file to see if we start the console window or not.
    // From now on we can use printf to output to the console window.

    if (isClientMode) {
		ToggleConsoleVisibility(L"SendFSKey - Client Console");
        printf("Client Console enabled.\n");
	}
    else {
        ToggleConsoleVisibility(L"SendFSKey - Server Console");
        printf("Server Console enabled.\n");
    }

    // Initialize Winsock for both client and server
    if (!initializeWinsock()) {
        return -1; // Exit if Winsock initialization fails
    }

    if (isClientMode) {

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"Microsoft Flight Simulator - SendFSKey Client";
        wchar_t const* className = L"AceApp";

        // Open our client window
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hInstance = hInstance;
        wc.lpszClassName = className;
        wc.lpfnWndProc = ClientWindowProc;
        wc.lpszMenuName = MAKEINTRESOURCE(IDR_CLIENTMENU);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

        if (!RegisterClass(&wc)) return -1;

        // Main Client Window
        HWND hWndClient = CreateWindowEx(
            0,                // No extended window styles.
            className,        // Pointer to registered class name.
            windowName,       // Pointer to window name.
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Window style.
            CW_USEDEFAULT,    // Use the system's default horizontal position.
            CW_USEDEFAULT,    // Use the system's default vertical position.
            DEFAULT_WIDTH,    // Use the system's default width.
            DEFAULT_HEIGHT,    // Use the system's default height.
            nullptr,          // No parent window.
            nullptr,          // Use the class menu.
            hInstance,        // Handle to application instance.
            nullptr           // No additional window data.
        );

        // Check if the Main Client Window was created successfully.
        if (!hWndClient) {
            return -1; // Handle main window creation failure.
        }

        // Create a static control for displaying text (replaces the edit control).
        HWND hStaticDisplay = CreateWindowEx(
            0,                  // No extended window styles.
            L"STATIC",          // STATIC control class.
            L"",                // Initial text - this will be changed dynamically.
            WS_CHILD | WS_VISIBLE | SS_LEFT, // Static control styles.
            8,                 // x-coordinate of the upper-left corner.
            8,                 // y-coordinate of the upper-left corner.
            DEFAULT_WIDTH,                // Width; adjusted for padding.
            DEFAULT_HEIGHT,                 // Height; enough for one line of text, adjust as needed.
            hWndClient,         // Handle to the parent window.
            (HMENU)IDC_MAIN_DISPLAY, // Control ID, ensure it's unique and doesn't conflict with existing IDs.
            hInstance,          // Handle to application instance.
            NULL                // No additional window data.
        );

        // Check if the static control was created successfully.
        if (!hStaticDisplay) {
            return -1; // Handle static control creation failure.
        }

        // Set the font for the static control to the modern Segoe UI.
        HFONT hFontStatic = CreateFont(
            FONT_SIZE,        // Font height.
            0,                // Average character width.
            0,                // Angle of escapement.
            0,                // Base-line orientation angle.
            FW_NORMAL,        // Font weight.
            FALSE,            // Italic attribute option.
            FALSE,            // Underline attribute option.
            FALSE,            // Strikeout attribute option.
            DEFAULT_CHARSET,  // Character set identifier.
            OUT_DEFAULT_PRECIS, // Output precision.
            CLIP_DEFAULT_PRECIS, // Clipping precision.
            CLEARTYPE_QUALITY,   // Output quality.
            VARIABLE_PITCH | FF_SWISS, // Pitch and family.
            L"Segoe UI"       // Font typeface name.
        );

        SendMessage(hStaticDisplay, WM_SETFONT, (WPARAM)hFontStatic, TRUE);
        
        // Async connection attempt
        if (!establishConnection()) {
            // Get the IP address of the server
            wprintf(L"Trying to establish a connection to IP Address: %s\n", serverIPconf.c_str());
        }
        else {
            wprintf(L"Connection already established\n");
		}

        // This is our window loop for the client
        wprintf(L"GUI WindowProcess loop starting for the client. We can now send messages to the client GUI.\n");
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    else if (!isClientMode) {

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"SendFSKey - Server";
        wchar_t const* className = L"BojoteApp";

        // Open our server window
        ws.hCursor = LoadCursor(nullptr, IDC_ARROW);
        ws.hInstance = hInstance;
        ws.lpszClassName = className;
        ws.lpfnWndProc = ServerWindowProc;
        ws.lpszMenuName = MAKEINTRESOURCE(IDC_SENDFSKEY_SERVER);
        ws.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

        if (!RegisterClass(&ws)) return -1;

        // Main Server Window
        HWND hWndServer = CreateWindowEx(
            0,                // Extended window styles.
            className,        // Pointer to registered class name.
            windowName,       // Pointer to window name.
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Window style.
            CW_USEDEFAULT,    // Use the system's default horizontal position.
            CW_USEDEFAULT,    // Use the system's default vertical position.
            DEFAULT_WIDTH,    // Use the system's default width.
            DEFAULT_HEIGHT,   // Use the system's default height.
            NULL,             // No parent window.
            NULL,             // No menu.
            hInstance,        // Handle to application instance.
            NULL              // No additional window data.
        );

        // Check if the Main Server Window was created successfully.
        if (!hWndServer) {
            return -1; // Handle main window creation failure.
        }

        // Create a static control for the server window to display messages.
        HWND hStaticServer = CreateWindowEx(
            0,                  // No extended window styles.
            L"STATIC",          // STATIC control class for display.
            L"",                // Initial text - can be updated dynamically.
            WS_CHILD | WS_VISIBLE | SS_LEFT, // Static control styles for text display.
            8,                 // x-coordinate of the upper-left corner.
            8,                 // y-coordinate of the upper-left corner.
            DEFAULT_WIDTH,      // Width of the control; adjusted for padding.
            DEFAULT_HEIGHT,     // Height of the control; enough for one line of text, adjust as needed.
            hWndServer,         // Handle to the parent window.
            (HMENU)IDC_STATIC_SERVER, // Control ID, use a unique identifier.
            hInstance,          // Handle to the instance.
            NULL                // No additional window data.
        );

        // Check if the static control was created successfully.
        if (!hStaticServer) {
            return -1; // Handle static control creation failure.
        }

        // Set the font for the static control to the modern Segoe UI.
        HFONT hFontStatic = CreateFont(
            FONT_SIZE,        // Font height.
            0,                // Average character width.
            0,                // Angle of escapement.
            0,                // Base-line orientation angle.
            FW_NORMAL,        // Font weight.
            FALSE,            // Italic attribute option.
            FALSE,            // Underline attribute option.
            FALSE,            // Strikeout attribute option.
            DEFAULT_CHARSET,  // Character set identifier.
            OUT_DEFAULT_PRECIS, // Output precision.
            CLIP_DEFAULT_PRECIS, // Clipping precision.
            CLEARTYPE_QUALITY,   // Output quality.
            VARIABLE_PITCH | FF_SWISS, // Pitch and family.
            L"Segoe UI"       // Font typeface name.
        );

        SendMessage(hStaticServer, WM_SETFONT, (WPARAM)hFontStatic, MAKELPARAM(TRUE, 0));

        if (isServerUp()) {
            wprintf(L"Could not start server. Port is already in use, restart your computer\n");
            MessageBox(NULL, L"Could not start server. Port is already in use, restart your computer.", L"Network Error", MB_ICONERROR | MB_OK);
            // return -1;  // This return should happen regardless of the DEBUG flag's state
        }
        else {
            std::wstring serverIP = getServerIPAddress();
            // Display the server IP address and port in the console

            std::wstring finalMessage = L"SendFSKey v1.0 - by Jesus \"Bojote\" Altuve - Free to use and distribute\r\n";
            finalMessage += L"\n";
            finalMessage += L"Server started successfully on ";
            finalMessage += L"IP Address: " + serverIP + L", "; // Correct usage of .c_str()
            finalMessage += L"using port: " + std::to_wstring(port) + L".\n";
            finalMessage += L"Ready to accept connections. Just run SendFSKey on remote computer in client mode\n";
            finalMessage += L"\n";

            // Start the server
            if (!initializeServer())
                MessageBox(NULL, L"Failed to start server. Try restarting this computer and if problem persists contact the author.", L"Network Error", MB_ICONERROR | MB_OK);
            else
                MessageBox(NULL, L"Server started succesfully", L"Connected", MB_ICONINFORMATION | MB_OK);

		}

        // This is the server window loop
        wprintf(L"GUI WindowProcess loop starting for the client. We can now send messages to the server GUI.\n");
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    else {
        MessageBoxA(NULL, "Mode not set. Application will now exit.", "Error", MB_ICONERROR);
        wprintf(L"Application exiting due to invalid input or incorrect ini file\n");
        return -1; // Exit application due to invalid input or incorrect ini file
    }

    // Cleanup before exit
    if (isClientMode) {
        closeClientConnection(); // For client
        wprintf(L"Closing client connection\n");
    }
    else {
        // For server mode, insert cleanup operations here
        wprintf(L"Closing server connection\n");
        cleanupServer();
    }
    cleanupWinsock();
    wprintf(L"Winsock cleanup and exiting program\n");
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
                serverIPconf = ipBuffer; // This is fine as both are now wide strings.
            }
            WriteSettingsToIniFile(mode, serverIPconf);
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
    case WM_APPEND_TEXT_TO_CONSOLE: {
        // Extract the text pointer from wParam and cast it back to std::wstring*
        std::wstring* pText = reinterpret_cast<std::wstring*>(wp);

        if (pText) {
            // Do the actual UI update here
            SetWindowText(hStaticDisplay, pText->c_str());

            // Don't forget to delete the allocated memory to avoid memory leaks
            delete pText;
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wp;
        SetBkMode(hdcStatic, TRANSPARENT); // Set background mode to transparent
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE); // Use system button face color
    }
    case WM_SIZE:
    {
        int padding = 8; // For example, a padding of 0 pixels

        // Get the new width and height of the window
        int width = LOWORD(lp) - 2 * padding;
        int height = HIWORD(lp) - 2 * padding;

        // Resize and reposition the control
        MoveWindow(hStaticDisplay, padding, padding, width, height, TRUE);
    }
    break;
    case WM_COMMAND: {
        int wmId = LOWORD(wp);
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDM_ABOUT_BOX), hWnd, AboutDlgProc);
            break;
        case ID_CLIENT_CONNECT: {
            closeClientConnection(); // Close the existing connection if any and re-establish
            if (!establishConnection()) {
                wprintf(L"Trying to establish a connection to IP Address: %s\n", serverIPconf.c_str());
            }
            else {
                wprintf(L"Connection was already established to IP Address: %s\n", serverIPconf.c_str());
			}
            break;
        }
        case ID_CLIENT_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_ENABLE_CONSOLE:
            ToggleConsoleVisibility(L"SendFSKey Console - Client");
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

        getKey(keyCodeNum, TRUE, TRUE); // Is system key and is key down

        break;
    }
    case WM_KEYDOWN: {
        // Extract the scan code from LPARAM
        UINT scanCode = (lp >> 16) & 0x00ff;
        UINT keyCodeNum = static_cast<UINT>(wp);

        getKey(keyCodeNum, FALSE, TRUE); // Is not system key and is key down

        break;
    }
    case WM_SYSKEYUP: {
        // Extract the scan code from LPARAM
        UINT scanCode = (lp >> 16) & 0x00ff;
        UINT keyCodeNum = static_cast<UINT>(wp);

        getKey(keyCodeNum, TRUE, FALSE); // Is not system key and is key up

        break;
    }
    case WM_KEYUP:
    {       // Extract the scan code from LPARAM
            UINT scanCode = (lp >> 16) & 0x00ff;
            UINT keyCodeNum = static_cast<UINT>(wp);

            getKey(keyCodeNum, FALSE, FALSE); // Is not system key and is key up

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
    case WM_APPEND_TEXT_TO_CONSOLE: {
        // Extract the text pointer from wParam and cast it back to std::wstring*
        std::wstring* pText = reinterpret_cast<std::wstring*>(wp);

        if (pText) {
            // Do the actual UI update here
            SetWindowText(hStaticDisplay, pText->c_str());

            // Don't forget to delete the allocated memory to avoid memory leaks
            delete pText;
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wp;
        SetBkMode(hdcStatic, TRANSPARENT); // Set background mode to transparent
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE); // Use system button face color
    }
    case WM_SIZE:
    {
        int padding = 8; // For example, a padding of 0 pixels

        // Get the new width and height of the window
        int width = LOWORD(lp) - 2 * padding;
        int height = HIWORD(lp) - 2 * padding;

        // Resize and reposition the control
        MoveWindow(hStaticServer, padding, padding, width, height, TRUE);
    }
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
            if (isServerUp()) {
                MessageBox(NULL, L"Server is already running", L"Error", MB_ICONERROR | MB_OK);
            }
            else {
				if (!initializeServer())
					MessageBox(NULL, L"Failed to start server. Try restarting this computer and if problem persists contact the author.", L"Network Error", MB_ICONERROR | MB_OK);
				else
					MessageBox(NULL, L"Server started succesfully", L"Connected", MB_ICONINFORMATION | MB_OK);
			}
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