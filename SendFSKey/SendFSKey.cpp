#include <Windows.h>
#include <strsafe.h>
#include <string>
#include <thread>
#include <sstream>
#include "framework.h"
#include "SendFSKey.h"
#include "Globals.h"
#include "utilities.h"
#include "NetworkClient.h"
#include "NetworkServer.h"

// Declare a brush for the static control background globally or at a suitable scope
HBRUSH hStaticBkBrush = CreateSolidBrush(RGB(240, 240, 240)); // Light grey background


// Function prototypes
LRESULT CALLBACK ServerWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClientWindowProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Global variables for the README.md dialog
// HWND hDlg = NULL; // Handle to the dialog box

// Global variables
HINSTANCE g_hInst = NULL;  // Definition
HINSTANCE g_hInst_client = NULL;  // Definition
HINSTANCE g_hInst_server = NULL;  // Definition

// Event handle for GUI ready
HANDLE guiReadyEvent = NULL; // Initialization at declaration

// Static control handles
HWND hStaticServer = NULL; // Handle to your server edit control
HWND hStaticClient = NULL; // Handle to your client edit control

// Default values for the client and server Windows
int PADDING = 10;
int DEFAULT_WIDTH = 570;
int DEFAULT_HEIGHT = 180;
int STATIC_DEFAULT_WIDTH = DEFAULT_WIDTH - (PADDING * 4);
int STATIC_DEFAULT_HEIGHT = DEFAULT_HEIGHT - (PADDING * 8);
int FONT_SIZE = 18;
std::wstring FONT_TYPE = L"Segoe UI Variable"; // was Segoe UI Variable

// Message loop for client and server
MSG msg_client = { 0 };
MSG msg_server = { 0 };

// App Window Initialization
WNDCLASS wc = { 0 };
WNDCLASS ws = { 0 };

// Initialize windowName and className
wchar_t const* windowName;
wchar_t const* className;

// Operation mode
bool isClientMode;

// Our ini file path
std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini"; // Build the full INI file path

// Initialize App Version Info
std::wstring companyName = GetSimpleVersionInfo(L"CompanyName");
std::wstring productName = GetSimpleVersionInfo(L"ProductName");
std::wstring legalCopyright = GetSimpleVersionInfo(L"LegalCopyright");
std::wstring productVersion = GetSimpleVersionInfo(L"ProductVersion");

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

    // Rich text library
    LoadLibrary(TEXT("Msftedit.dll"));

    if (isAlreadyRunning()) {
		MessageBox(NULL, L"SendFSKey is already running. Please close the existing instance before starting a new one.", L"Error", MB_ICONERROR);
		return -1;
	}

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
            MessageBox(NULL, message.c_str(), L"Error", MB_ICONERROR);
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
            MessageBox(NULL, L"Selected mode has not been implemented. Will quit now", L"Error", MB_ICONERROR | MB_OK);
            // Also, delete the init file as we have no idea what the user has entered
            DeleteIniFileAndRestart();
        }
    }

    // Get Visibility setting
    wchar_t visibilityBuffer[5]; // Enough for "Yes" or "No"
    GetPrivateProfileStringW(L"Settings", L"ConsoleVisibility", L"Yes", visibilityBuffer, 5, iniPath.c_str());
    std::wstring consoleVisibility = visibilityBuffer;

    // Get Queuing setting
    wchar_t use_queuingBuffer[5]; // Enough for "true" or "false"
    GetPrivateProfileStringW(L"Settings", L"UseQueing", L"No", use_queuingBuffer, 5, iniPath.c_str());
    use_queuing = use_queuingBuffer;

    if (use_queuing == L"Yes") {
        queueKeys = TRUE;
        maxQueueSize = 4;
    }
    else {
        queueKeys = FALSE;
        maxQueueSize = 0;
    }

    // Get Target Window setting
    wchar_t target_windowBuffer[256]; // Enough for long names
    GetPrivateProfileStringW(L"Settings", L"TargetWindow", L"AceApp", target_windowBuffer, 256, iniPath.c_str());
    target_window = target_windowBuffer;
    
    // Get App process name
    wchar_t app_processBuffer[256]; // Enough for long names
    GetPrivateProfileStringW(L"Settings", L"AppProcess", L"FlightSimulator.exe", app_processBuffer, 256, iniPath.c_str());
    app_process = app_processBuffer;

    // Adjust the buffer size to 2 to accommodate one character plus a null terminator
    wchar_t DEBUGBuffer[2]; // Enough for "0" or "1" plus a null terminator
    GetPrivateProfileStringW(L"Settings", L"Debug", L"0", DEBUGBuffer, _countof(DEBUGBuffer), iniPath.c_str());
    DEBUG = (DEBUGBuffer[0] == L'1');

    // isMinimized or not?
    wchar_t start_minimizedBuffer[5]; // Enough for long names
    GetPrivateProfileStringW(L"Settings", L"StartMinimized", L"No", start_minimizedBuffer, 5, iniPath.c_str());
    start_minimized = start_minimizedBuffer;

    int exitCode = 0; // Default exit code

    // Dont Modify anything BELOW this line...

    if (isClientMode) {

        // Show the console window
        if (consoleVisibility == L"Yes") ToggleConsoleVisibility(L"SendFSKey - Client Console");

        if (DEBUG)
            wprintf(L" *** CLIENT DEBUG MODE ENABLED ***\n");

        // Initialize Winsock for both client and server
        if (!initializeWinsock()) {
            return -1; // Exit if Winsock initialization fails
        }

        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"Microsoft Flight Simulator - SendFSKey Client";
        wchar_t const* className = L"AceApp";

        // Open our client window
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hInstance = hInstance;
        wc.lpszClassName = className;
        wc.lpfnWndProc = ClientWindowProc;
        wc.lpszMenuName = MAKEINTRESOURCE(IDR_CLIENTMENU);
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SENDFSKEY));
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

        if (!RegisterClass(&wc)) return -1;

        // Main Client Window
        HWND hWndClient = CreateWindowEx(
            WS_EX_OVERLAPPEDWINDOW,             // Client edge for a sunken border.
            className,                          // Pointer to registered class name.
            windowName,                         // Pointer to window name.
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,   // Window style.
            CW_USEDEFAULT,                      // Use the system's default horizontal position.
            CW_USEDEFAULT,                      // Use the system's default vertical position.
            DEFAULT_WIDTH,                      // Use the system's default width.
            DEFAULT_HEIGHT,                     // Use the system's default height.
            nullptr,                            // No parent window.
            nullptr,                            // Use the class menu.
            hInstance,                          // Handle to application instance.
            nullptr                             // No additional window data.
        );

        // Check if the Main Client Window was created successfully.
        if (!hWndClient) {
            wprintf(L"Could not create main window for server\n");
            return -1; // Handle main window creation failure.
        }

        // Create a static control for displaying text (replaces the edit control).
        hStaticClient = CreateWindowEx(
            WS_EX_WINDOWEDGE,                   // No extended window styles.
            L"STATIC",                          // STATIC control class.
            L"",                                // Initial text - this will be changed dynamically.
            WS_CHILD | WS_VISIBLE | SS_LEFT,    // Static control styles.
            PADDING,                            // x-coordinate of the upper-left corner.
            PADDING,                            // y-coordinate of the upper-left corner.
            STATIC_DEFAULT_WIDTH,               // Width; adjusted for padding.
            STATIC_DEFAULT_HEIGHT,              // Height; adjusted for padding.
            hWndClient,                         // Handle to the parent window.
            (HMENU)IDC_MAIN_DISPLAY_TEXT,       // Control ID, ensure it's unique and doesn't conflict with existing IDs.
            hInstance,                          // Handle to application instance.
            NULL                                // No additional window data.
        );

        // Check if the static control was created successfully.
        if (!hStaticClient) {
            wprintf(L"Could not create static control for client\n");
            return -1; // Handle static control creation failure.
        }

        HFONT hFontStatic = CreateFont(
            FONT_SIZE,                           // Increased font height for better readability.
            0,                                   // Average character width.
            0,                                   // Angle of escapement.
            0,                                   // Base-line orientation angle.
            FW_NORMAL,                           // Font weight.
            FALSE,                               // Italic attribute option.
            FALSE,                               // Underline attribute option.
            FALSE,                               // Strikeout attribute option.
            DEFAULT_CHARSET,                     // Character set identifier.
            OUT_DEFAULT_PRECIS,                  // Output precision.
            CLIP_DEFAULT_PRECIS,                 // Clipping precision.
            CLEARTYPE_QUALITY,                   // Output quality.
            VARIABLE_PITCH | FF_SWISS,           // Pitch and family.
            FONT_TYPE.c_str()                    // Try the new "Segoe UI Variable" font.
        );

        // Check if the Main Client Window was created successfully.
        if (!hFontStatic) {
            wprintf(L"Could not create static font control for client\n");
            return -1; // Handle main window creation failure.
        }

        SendMessage(hStaticClient, WM_SETFONT, (WPARAM)hFontStatic, TRUE);

        // Create an event to signal when the GUI is ready
        guiReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Manual-reset event, initially non-signaled
        if (guiReadyEvent == NULL) {
            wprintf(L"Could not create the guiReady event\n");
        }

        // After setting up the GUI and just before starting the message loop:
        if (guiReadyEvent != NULL) {
            SetEvent(guiReadyEvent);
        }

        if (guiReadyEvent != NULL) {
            WaitForSingleObject(guiReadyEvent, INFINITE);
        }
        else {
            wprintf(L"Error with the GUI ready event.\n");
        }

        // Right after loading settings or during window initialization
        HMENU hMenu = GetMenu(hWndClient);
        if (start_minimized == L"Yes") {
            ShowWindow(hWndClient, SW_SHOWMINIMIZED);
            CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | MF_CHECKED);
        }
        else {
            ShowWindow(hWndClient, SW_SHOWNORMAL);
            CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | MF_UNCHECKED);
        }

        // Se we dont lock the GUI thread, we will start the connection in a separate thread
        clientConnectionThread(); // Start the client connection

        // This is our window loop for the client
        if(DEBUG) wprintf(L"GUI WindowProcess loop starting for the client. We can now send messages to the client GUI.\n");

        while (GetMessage(&msg_client, nullptr, 0, 0)) {
            TranslateMessage(&msg_client);
            DispatchMessage(&msg_client);
        }

        closeClientConnection(); // For client
        wprintf(L"Closing client connection\n");

        cleanupWinsock();
        wprintf(L"Winsock cleanup and exiting program\n");

        // Set exit code from client message loop
        exitCode = (int)msg_client.wParam;
    }
    else if (!isClientMode) {

        if (consoleVisibility == L"Yes") ToggleConsoleVisibility(L"SendFSKey - Server Console");

        if (DEBUG)
            wprintf(L" *** SERVER DEBUG MODE ENABLED ***\n");

        // Initialize Winsock for both client and server
        if (!initializeWinsock()) {
            return -1; // Exit if Winsock initialization fails
        }

        // Check if the application has sufficient privileges
        CheckApplicationPrivileges();

        if (!has_permission) {
            MessageBox(NULL, L"Insufficient privileges to send input to Flight Simulator. Please run this application as an administrator or add the application to your Flight Simulator exe.xml.", L"Permission Error", MB_ICONERROR);
        }



        // Default values for windowName and className, will be set conditionally below if we need to change them
        wchar_t const* windowName = L"SendFSKey - Server";
        wchar_t const* className = L"BojoteApp";

        // Open our server window
        ws.hCursor = LoadCursor(nullptr, IDC_ARROW);
        ws.hInstance = hInstance;
        ws.lpszClassName = className;
        ws.lpfnWndProc = ServerWindowProc;
        ws.lpszMenuName = MAKEINTRESOURCE(IDC_SENDFSKEY_SERVER);
        ws.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SENDFSKEY));
        ws.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

        if (!RegisterClass(&ws)) return -1;

        // Main Server Window
        HWND hWndServer = CreateWindowEx(
            WS_EX_OVERLAPPEDWINDOW,             // Client edge for a sunken border.
            className,                          // Pointer to registered class name.
            windowName,                         // Pointer to window name.
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,   // Window style.
            CW_USEDEFAULT,                      // Use the system's default horizontal position.
            CW_USEDEFAULT,                      // Use the system's default vertical position.
            DEFAULT_WIDTH,                      // Use the system's default width.
            DEFAULT_HEIGHT,                     // Use the system's default height.
            nullptr,                            // No parent window.
            nullptr,                            // Use the class menu.
            hInstance,                          // Handle to application instance.
            nullptr                             // No additional window data.
        );

        // Check if the Main Server Window was created successfully.
        if (!hWndServer) {
            wprintf(L"Could not create main window for server\n");
            return -1; // Handle main window creation failure.
        }

        // Create a static control for the server window to display messages.
        hStaticServer = CreateWindowEx(
            WS_EX_WINDOWEDGE,                   // No extended window styles.
            L"STATIC",                          // STATIC control class.
            L"",                                // Initial text - this will be changed dynamically.
            WS_CHILD | WS_VISIBLE | SS_LEFT,    // Static control styles.
            PADDING,                            // x-coordinate of the upper-left corner.
            PADDING,                            // y-coordinate of the upper-left corner.
            STATIC_DEFAULT_WIDTH,               // Width; adjusted for padding.
            STATIC_DEFAULT_HEIGHT,              // Height; adjusted for padding.
            hWndServer,                         // Handle to the parent window.
            (HMENU)IDC_MAIN_DISPLAY_TEXT,       // Control ID, ensure it's unique and doesn't conflict with existing IDs.
            hInstance,                          // Handle to application instance.
            NULL                                // No additional window data.
        );

        // Check if the static control was created successfully.
        if (!hStaticServer) {
            wprintf(L"Could not create static control for server\n");
            return -1; // Handle static control creation failure.
        }

        // Set the font for the static control to the modern Segoe UI.
        HFONT hFontStatic = CreateFont(
            FONT_SIZE,                           // Increased font height for better readability.
            0,                                   // Average character width.
            0,                                   // Angle of escapement.
            0,                                   // Base-line orientation angle.
            FW_NORMAL,                           // Font weight.
            FALSE,                               // Italic attribute option.
            FALSE,                               // Underline attribute option.
            FALSE,                               // Strikeout attribute option.
            DEFAULT_CHARSET,                     // Character set identifier.
            OUT_DEFAULT_PRECIS,                  // Output precision.
            CLIP_DEFAULT_PRECIS,                 // Clipping precision.
            CLEARTYPE_QUALITY,                   // Output quality.
            VARIABLE_PITCH | FF_SWISS,           // Pitch and family.
            FONT_TYPE.c_str()                    // Try the new "Segoe UI Variable" font.
        );

        // Check if the static control was created successfully.
        if (!hFontStatic) {
            wprintf(L"Could not create static font control for server\n");
            return -1; // Handle static control creation failure.
        }

        SendMessage(hStaticServer, WM_SETFONT, (WPARAM)hFontStatic, MAKELPARAM(TRUE, 0));

        // Create an event to signal when the GUI is ready
        guiReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Manual-reset event, initially non-signaled
        if (guiReadyEvent == NULL) {
            wprintf(L"Could not create the guiReady event\n");
        }

        // After setting up the GUI and just before starting the message loop:
        if (guiReadyEvent != NULL) {
            SetEvent(guiReadyEvent);
        }

        if (guiReadyEvent != NULL) {
            WaitForSingleObject(guiReadyEvent, INFINITE);
        }
        else {
            wprintf(L"Error with the GUI ready event.\n");
        }
         
        // After loading settings and updating UI checkmarks
        if (start_minimized == L"Yes") {
            MinimizeToTray(hWndServer);
            ShowWindow(hWndServer, SW_HIDE); // Hide the window
        }

        UpdateMenuCheckMarks(hWndServer); // Update the menu checkmarks

        // Start the server in a separate thread to avoid blocking the main thread
        serverStartThread(); // Start the server in a separate thread

        // This is the server window loop
        if (DEBUG) wprintf(L"GUI WindowProcess loop starting. We can now send messages to the server GUI.\n");

        // This will pause (in a separate thread) until the parent process exits, 
        // then will resume but since we did PostQuitMessage(0) inside the thread the line below that enters the message 
        // loop will exit immediately and the application will close after cleaning up.
        monitorParentProcess(); // Monitor the parent process

        while (GetMessage(&msg_server, nullptr, 0, 0)) {
            TranslateMessage(&msg_server);
            DispatchMessage(&msg_server);
        }

        // For server mode, insert cleanup operations here
        wprintf(L"Closing server connection\n");
        cleanupServer();

        // Set exit code from server message loop
        exitCode = (int)msg_server.wParam;
    }
    else {
        MessageBox(NULL, L"Mode not set. Application will now exit.", L"Error", MB_ICONERROR);
        wprintf(L"Application exiting due to invalid input or incorrect ini file\n");
        return -1; // Exit application due to invalid input or incorrect ini file
    }

    if (guiReadyEvent != NULL)
        CloseHandle(guiReadyEvent); // Close the event handle

    return exitCode; // Return the exit code
}

INT_PTR CALLBACK ExperimentoDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        LoadTextFromResourceToEditControl(hDlg, IDC_TEXT_DISPLAY);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            DestroyWindow(hDlg); // Instead of EndDialog
            // EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {

        SetDlgItemTextW(hDlg, IDC_COMPANYNAME, companyName.c_str());
        SetDlgItemTextW(hDlg, IDC_APPNAME, productName.c_str());
        SetDlgItemTextW(hDlg, IDC_APPCOPYRIGHT, legalCopyright.c_str());
        SetDlgItemTextW(hDlg, IDC_APPVERSION, productVersion.c_str());

        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SENDFSKEY));
        if (hIcon)
        {
            SendDlgItemMessage(hDlg, IDC_MYICON, STM_SETICON, (WPARAM)hIcon, 0);
        }
        return (INT_PTR)TRUE;
    }
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

        // Set the default selection to Server
        SendMessage(hwndCombo, CB_SETCURSEL, 1, 0); // Select "Server" by default which is the one users should always install FIRST

        // Hide elements by default
        ShowWindow(GetDlgItem(hDlg, IDC_IPADDRESS), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_IPADDRESSTEXT), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_IPADDRTEXT), SW_HIDE);

        return TRUE; // Return TRUE to set the keyboard focus to the control specified by wParam.
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {


        case IDC_COMBOBOX: // The ID of your combo box control
            if (HIWORD(wParam) == CBN_SELCHANGE) {

                    HWND hwndComboBox = GetDlgItem(hDlg, IDC_COMBOBOX);
                    int selected = (int)SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);

                    HWND hwndIPAddr = GetDlgItem(hDlg, IDC_IPADDRESS);
                    HWND hwndIPAddrTxt = GetDlgItem(hDlg, IDC_IPADDRESSTEXT);
                    HWND hwndIPAddrTxtInfo = GetDlgItem(hDlg, IDC_IPADDRTEXT);
                    HWND hwndServerTxtInfo = GetDlgItem(hDlg, IDC_SERVERMODE_TEXT);

                    // Assuming index '0' is the one where IP should be shown
                    if (selected == 0) {
                        ShowWindow(hwndIPAddr, SW_SHOW);
                        ShowWindow(hwndIPAddrTxt, SW_SHOW);
                        ShowWindow(hwndIPAddrTxtInfo, SW_SHOW);
                        ShowWindow(hwndServerTxtInfo, SW_HIDE);
                    }
                    else {
                        ShowWindow(hwndServerTxtInfo, SW_SHOW);
                        ShowWindow(hwndIPAddr, SW_HIDE);
                        ShowWindow(hwndIPAddrTxt, SW_HIDE);
                        ShowWindow(hwndIPAddrTxtInfo, SW_HIDE);
                    }
            }
            break;
        case IDC_OK: {
            // Change the buffer to a wide string buffer.
            wchar_t modeBuffer[7] = { 0 };
            GetDlgItemText(hDlg, IDC_COMBOBOX, modeBuffer, _countof(modeBuffer)); // Use the wide-char version of GetDlgItemText
            mode = modeBuffer; // Now modeBuffer is a wide string, and this should work.

            if (mode == L"Client") { // Note the 'L' prefix to create a wide string literal.
                wchar_t ipBuffer[16] = { 0 };
                GetDlgItemText(hDlg, IDC_IPADDRESS, ipBuffer, _countof(ipBuffer)); // Again, make sure this is the wide-char version.
                serverIPconf = ipBuffer; // This is fine as both are now wide strings.

                std::wstring failMessage = L"Invalid IP address " + serverIPconf + L". Please enter the address of the computer where SendFSKey was installed in \'Server Mode\'";
                if (!IsValidIPv4(serverIPconf)) {
                    MessageBox(hDlg, failMessage.c_str(), L"ERROR", MB_ICONERROR);
                    return TRUE; // Prevent dialog from closing
                }
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
    case WM_COMMAND: {
        int wmId = LOWORD(wp);
        switch (wmId) {
        case IDD_HELP_EXPERIMENTO: {

            HWND hDlg = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_EXPERIMENTO), hWnd, ExperimentoDlgProc, 0);
            if (hDlg != NULL) {
                ShowWindow(hDlg, SW_SHOW); // Make sure the dialog is shown

            }
            else {
                MessageBox(NULL, L"Exiting", L"Exiting", MB_OK);
            }

            // CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EXPERIMENTO), hWnd, ExperimentoDlgProc, 0);
            // DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EXPERIMENTO), hWnd, ExperimentoDlgProc);
            break;

        }

        case ID_HELP_DISCORD:
            ShellExecute(NULL, L"open", L"https://discord.gg/3HDGcaP5VH", NULL, NULL, SW_SHOWNORMAL);
            break;
        case ID_HELP_GITHUB:
            ShellExecute(NULL, L"open", L"https://github.com/BojoteX/SendFSKey", NULL, NULL, SW_SHOWNORMAL);
            break;


        case ID_OPTIONS_MINIMIZEONSTART: {
            if (start_minimized == L"Yes") {
                start_minimized = L"No";
            }
            else {
                start_minimized = L"Yes";
            }

            // Write the setting to the INI file
            std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini";
            WritePrivateProfileStringW(L"Settings", L"StartMinimized", start_minimized.c_str(), iniPath.c_str());

            // Update the menu checkmark
            HMENU hMenu = GetMenu(hWnd);
            UINT state = GetMenuState(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND);
            if (state & MF_CHECKED) {
                CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | MF_UNCHECKED);
            }
            else {
                CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | MF_CHECKED);
            }
            break;
		}
        case IDM_ABOUT:
            DialogBox(g_hInst_client, MAKEINTRESOURCE(IDM_ABOUT_BOX), hWnd, AboutDlgProc);
            break;
        case ID_CLIENT_CONNECT: {
            closeClientConnection(); // Close the existing connection if any and re-establish
            clientConnectionThread(); // Start the client connection
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
    case WM_CLOSE: {
        closeClientConnection(); // For client;
        // Destroy the window to close it
        DestroyWindow(hWnd);
        return 0; // Indicate that we handled the message
    }
    case WM_DESTROY:

        if (guiReadyEvent != NULL)
            CloseHandle(guiReadyEvent);

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}

LRESULT CALLBACK ServerWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_SIZE:
        if (wp == SIZE_MINIMIZED && start_minimized == L"Yes") {
            MinimizeToTray(hWnd);
            ShowWindow(hWnd, SW_HIDE); // Hide the window
        }
        break;
    case WM_TRAYICON: {

		if (lp == WM_LBUTTONDBLCLK) {
			ShowWindow(hWnd, SW_RESTORE);
			// Bring the window to the foreground
			SetForegroundWindow(hWnd);
			return TRUE;
		}
        else if (LOWORD(lp) == WM_RBUTTONDOWN) {
            POINT curPoint;
            GetCursorPos(&curPoint);
            SetForegroundWindow(hWnd);

            // Display the context menu
            // This function is described in Step 2
            ShowContextMenu(hWnd, curPoint);

            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wp; // Get the device context for the static control
        SetBkColor(hdcStatic, RGB(240, 240, 240)); // Set the background color
        SetTextColor(hdcStatic, RGB(0, 0, 0)); // Set the text color, e.g., to black

        return (INT_PTR)hStaticBkBrush; // Return the brush used for the control's background
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wp); // Move this line inside the WM_COMMAND case
        switch (wmId) {


        case IDD_HELP_EXPERIMENTO: {

            HWND hDlg = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_EXPERIMENTO), hWnd, ExperimentoDlgProc, 0);
            if (hDlg != NULL) {
                ShowWindow(hDlg, SW_SHOW); // Make sure the dialog is shown

            }
            else {
                MessageBox(NULL, L"Exiting", L"Exiting", MB_OK);
            }

            // CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EXPERIMENTO), hWnd, ExperimentoDlgProc, 0);
            // DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EXPERIMENTO), hWnd, ExperimentoDlgProc);
            break;
        }

        case ID_HELP_DISCORD:
            ShellExecute(NULL, L"open", L"https://discord.gg/3HDGcaP5VH", NULL, NULL, SW_SHOWNORMAL);
            break;
        case ID_HELP_GITHUB:
            ShellExecute(NULL, L"open", L"https://github.com/BojoteX/SendFSKey", NULL, NULL, SW_SHOWNORMAL);
            break;

        case ID_OPTIONS_MINIMIZEONSTART: {
            if (start_minimized == L"Yes") {
                start_minimized = L"No";
            }
            else {
                start_minimized = L"Yes";
            }

            // Write the setting to the INI file
            std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini";
            WritePrivateProfileStringW(L"Settings", L"StartMinimized", start_minimized.c_str(), iniPath.c_str());

            // Update the menu checkmark
            HMENU hMenu = GetMenu(hWnd);
            UINT state = GetMenuState(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND);
            if (state & MF_CHECKED) {
                CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | MF_UNCHECKED);
            }
            else {
                CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | MF_CHECKED);
            }
            break;
        }

        case ID_TRAY_EXIT:
            // Perform the action for the "Exit" menu item
            DestroyWindow(hWnd);
            break;
        case ID_TRAY_OPEN:
            ShowWindow(hWnd, SW_RESTORE);
            // Bring the window to the foreground
            SetForegroundWindow(hWnd);
            break;
        case ID_TRAY_CONSOLE:
            ToggleConsoleVisibility(L"SendFSKey Server Console");
            break;
        case IDM_ABOUT:
            DialogBox(g_hInst_server, MAKEINTRESOURCE(IDM_ABOUT_BOX), hWnd, AboutDlgProc);
            break;
        case IDM_ENABLE_CONSOLE:
            ToggleConsoleVisibility(L"SendFSKey Server Console");
            break;
        case IDM_RESET_SETTINGS:
            DeleteIniFileAndRestart();
            break;
        case ID_SERVER_CONNECT:
            if (isServerUp()) {

                // Fetch the IP address as a std::wstring
                std::wstring serverIP = getServerIPAddress();

                // Create the message to be sent
                std::wstring portStr = std::to_wstring(port);
                std::wstring message = L"Server is already running on IP address " + serverIP + L" TCP port " + portStr;
                SendMessage(hStaticServer, WM_SETTEXT, 0, (LPARAM)message.c_str());

				}
            else {
                serverStartThread(); // Start the server in a separate thread
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
    case WM_CLOSE: {
        // serverRunning = false;
        // cleanupServer();
        // Destroy the window to close it
        DestroyWindow(hWnd);
        return 0; // Indicate that we handled the message
    }
    case WM_DESTROY:
        if (guiReadyEvent != NULL)
            CloseHandle(guiReadyEvent);

        PostQuitMessage(0); // Signal to end the application
        break;
    default:
        return DefWindowProc(hWnd, msg, wp, lp); // Default message processing
    }
    return 0; // Include a return statement here as good practice
}