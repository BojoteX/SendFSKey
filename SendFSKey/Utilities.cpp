#include <Windows.h>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <Richedit.h>
#include <regex>
#include "Globals.h"
#include "Utilities.h"
#include "NetworkClient.h"
#include "NetworkServer.h"

// .ini Settings defaults
int port = 8028; // Default port can be changed in the INI file
std::wstring mode = L"Server";
std::wstring serverIPconf = L"0.0.0.0";
std::wstring target_window = L"AceApp";
std::wstring app_process = L"FlightSimulator.exe";
std::wstring use_queuing = L"No";
std::wstring consoleVisibility = L"No";
std::wstring start_minimized = L"No";

// .ini file settings definitions only
bool queueKeys;
int maxQueueSize; // Maximum number of keys to queue, its hardcoded to 4 for now

// Global scope or within a class/structure as needed
NOTIFYICONDATA nid = {}; // Zero-initialize the structure

// Debug mode (.ini file will override this)
bool DEBUG = FALSE;

// This allows us to restart the application wuthout having to close the current instance
HANDLE mutexHandle = NULL; // Initialization to ensure it starts as NULL

// Global variable to check if the application has permission to send keys and check if the process is running
bool is_flightsimulator_running = false;
bool has_permission = false;

// Required for the console window
const size_t MAX_BUFFER_SIZE = 1024 * 10; // 10 KB, adjust as necessary, this is needed for the static control

// Below are the key codes for the keys that are not standard and required conversion
std::unordered_map<UINT, UINT> keyDownToUpMapping;

// Function to get the %appdata%/Local/SendFSKey directory path
std::wstring GetAppDataLocalSendFSKeyDir() {
    wchar_t appDataPath[MAX_PATH];
    // Use CSIDL_LOCAL_APPDATA instead of CSIDL_APPDATA to get the local app data folder
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring sendFSKeyPath = std::wstring(appDataPath) + L"\\SendFSKey";
        // Check if the directory exists
        if (GetFileAttributesW(sendFSKeyPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            // Directory does not exist, create it

            if (!CreateDirectoryW(sendFSKeyPath.c_str(), NULL)) {
                DWORD error = GetLastError();
                if (error != ERROR_ALREADY_EXISTS) {
                    MessageBox(NULL, L"There was an error creating the configuration file.", L"Error", MB_ICONERROR | MB_OK);
                }
            }
        }
        return sendFSKeyPath;
    }
    // Fallback in case of failure
    return L"";
}

bool IniFileExists(const std::string& filename) {
    std::wstring appDataPath = GetAppDataLocalSendFSKeyDir();
    std::wstring fullPath = appDataPath + L"\\" + std::wstring(filename.begin(), filename.end());
    std::ifstream ifile(fullPath);
    return ifile.good();
}

void WriteSettingsToIniFile(const std::wstring& mode, const std::wstring& ip) {
    std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini"; // Build the full INI file path
    if (!WritePrivateProfileStringW(L"Settings", L"Mode", mode.c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
    if (mode == L"Client") {
        if (!WritePrivateProfileStringW(L"Settings", L"IP", ip.c_str(), iniPath.c_str())) {
            DWORD error = GetLastError();
        }
    }

    // Default options
    if (!WritePrivateProfileStringW(L"Settings", L"TargetWindow", target_window.c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

    if (!WritePrivateProfileStringW(L"Settings", L"AppProcess", app_process.c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

    if (!WritePrivateProfileStringW(L"Settings", L"UseQueing", use_queuing.c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

    if (!WritePrivateProfileStringW(L"Settings", L"Port", std::to_wstring(port).c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

    if (!WritePrivateProfileStringW(L"Settings", L"Debug", std::to_wstring(DEBUG).c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

    if (!WritePrivateProfileStringW(L"Settings", L"ConsoleVisibility", consoleVisibility.c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

    if (!WritePrivateProfileStringW(L"Settings", L"StartMinimized", start_minimized.c_str(), iniPath.c_str()))
        DWORD error = GetLastError();

}

void getKey(UINT keyCodeNum, bool isSystemKey, bool isKeyDown) {

    if (isKeyDown) {
        printf("\n* [KEY_DOWN] DETECTED from Client Keyboard: (%d)", keyCodeNum);
    }
    else {
        printf("* [KEY_UP] DETECTED from Client Keyboard: (%d)", keyCodeNum);
    }

    if (isSystemKey)
        printf(" as SYSTEM key ");
    else {
        printf(" as STANDARD key ");
    }

    // Attempt to use the mapped key code for KEY_UP events
    if (!isKeyDown && keyDownToUpMapping.find(keyCodeNum) != keyDownToUpMapping.end()) {
        keyCodeNum = keyDownToUpMapping[keyCodeNum]; // Use the converted code
        keyDownToUpMapping.erase(keyCodeNum); // Clean up after use
        printf("and converted keyCode for KEY_UP: (%d)", keyCodeNum);
    }
    else if (isKeyDown) {
        // Process conversion only on KEY_DOWN
        UINT originalKeyCode = keyCodeNum; // Preserve original keycode
        if (keyCodeNum == VK_SHIFT) {
            if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
                keyCodeNum = 160; // Left Shift
            }
            else if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) {
                keyCodeNum = 161; // Right Shift
            }
        }
        else if (keyCodeNum == VK_CONTROL) {
            if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
                keyCodeNum = 162; // Left Control
            }
            else if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) {
                keyCodeNum = 163; // Right Control
            }
        }
        else if (keyCodeNum == VK_MENU) {
            if (GetAsyncKeyState(VK_LMENU) & 0x8000) {
                keyCodeNum = 164; // Left Menu (Alt)
            }
            else if (GetAsyncKeyState(VK_RMENU) & 0x8000) {
                keyCodeNum = 165; // Right Menu (Alt)
            }
        }

        // If a conversion occurred, map the original to the converted keycode
        if (originalKeyCode != keyCodeNum) {
            keyDownToUpMapping[originalKeyCode] = keyCodeNum;
            printf("and converted to: (%d)", keyCodeNum);
        }
    }

    if (isKeyDown) {
        printf("\n[KEY_DOWN] was SENT to Server using keycode: (%d)\n", keyCodeNum);
    }
    else {
        printf("\n[KEY_UP] was SENT to Server using keycode: (%d)\n", keyCodeNum);
    }

    // Proceed to send the key press with possibly updated keyCodeNum
    sendKeyPress(keyCodeNum, isKeyDown);
}

void ToggleConsoleVisibility(const std::wstring& title) {
    HWND consoleWindow = GetConsoleWindow();
    std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini";

    if (consoleWindow == NULL) {
        // No console attached, try to create one and show it
        if (AllocConsole()) {
            SetConsoleTitleW(title.c_str());
            freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

            if (isClientMode)
                printf("\n[CLIENT CONSOLE ENABLED]\n\n");
            else
                printf("\n[SERVER CONSOLE ENABLED]\n\n");

            // Update INI to reflect console is now visible
            WritePrivateProfileStringW(L"Settings", L"ConsoleVisibility", L"Yes", iniPath.c_str());
        }
    }
    else {
        // Toggle visibility
        bool isVisible = ::IsWindowVisible(consoleWindow) != FALSE;
        ShowWindow(consoleWindow, isVisible ? SW_HIDE : SW_SHOW);
        // Update INI to reflect the new visibility state
        WritePrivateProfileStringW(L"Settings", L"ConsoleVisibility", isVisible ? L"No" : L"Yes", iniPath.c_str());
    }
}

void RestartApplication() {
    // Ensure all cleanup is done before restarting, including releasing the mutex
    if (mutexHandle != NULL) {
        ReleaseMutex(mutexHandle);
        CloseHandle(mutexHandle);
        mutexHandle = NULL; // Ensure the handle is marked as released.

        // Cleanup operations
        if (isClientMode) {
            closeClientConnection(); // For client
            wprintf(L"Closing client connection\n");
        }
        else {
            // For server mode
            wprintf(L"Closing server connection\n");
            cleanupServer();
        }
        wprintf(L"Winsock cleanup and exiting program\n");
        cleanupWinsock();
    }

    wchar_t szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) != 0) {
        // Launch the application again
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOASYNC;
        sei.lpVerb = L"open";
        sei.lpFile = szPath;
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteEx(&sei)) {
            DWORD error = GetLastError();
            MessageBox(NULL, L"Failed to restart the application.", L"Error", MB_ICONERROR | MB_OK);
            // Re-acquire the mutex for the current (failing) instance since restart didn't happen
            mutexHandle = CreateMutex(NULL, FALSE, L"Global\\MyUniqueAppNameMutex");
            return;
        }

        // Exit the current instance of the application gracefully
        exit(0);
    }
    else {
        MessageBox(NULL, L"Failed to obtain application path.", L"Error", MB_ICONERROR | MB_OK);
    }
}

bool ConfirmResetAndRestart() {
    int response = MessageBox(NULL,
        L"Reset settings and restart the application?",
        L"Confirm Reset",
        MB_ICONQUESTION | MB_YESNO);
    return (response == IDYES);
}

void DeleteIniFileAndRestart() {
    // Prompt the user for confirmation
    if (!ConfirmResetAndRestart()) {
        return; // User did not confirm, exit the function
    }

    std::wstring iniPath = GetAppDataLocalSendFSKeyDir() + L"\\SendFSKey.ini";

    // Check if the file exists
    if (IniFileExists("SendFSKey.ini")) {
        // Delete the file
        wprintf(L"Deleting .ini file\n");
        if (!DeleteFileW(iniPath.c_str())) {
            DWORD error = GetLastError();
            MessageBox(NULL, L"Failed to delete the configuration file.", L"Error", MB_ICONERROR | MB_OK);
            wprintf(L"Failed to delete .ini file\n");
            return; // Exit the function if unable to delete the file
        }
    }

    // Restart the application
    RestartApplication();
}

std::wstring GetSimpleVersionInfo(const std::wstring& infoType) {
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) {
        return L"Failed to get module file name";
    }

    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW(exePath, &dummy);
    if (size == 0) {
        return L"Version info size not found";
    }

    std::vector<char> data(size);
    if (!GetFileVersionInfoW(exePath, 0, size, data.data())) {
        return L"Failed to get version info";
    }

    void* pBuf;
    UINT len;
    std::wstring subBlock = L"\\StringFileInfo\\040904b0\\" + infoType;
    if (!VerQueryValueW(data.data(), subBlock.c_str(), &pBuf, &len)) {
        return L"Information not found";
    }

    return std::wstring(reinterpret_cast<wchar_t*>(pBuf));
}

bool IsValidIPv4(const std::wstring& ip) {
    // Immediately exclude "0.0.0.0" if it's considered invalid for your application
    if (ip == L"0.0.0.0") {
        return false;
    }

    int numDots = 0;
    size_t start = 0;
    size_t end = ip.find(L'.');

    // Ensure there are exactly three dots in the IP address
    if (std::count(ip.begin(), ip.end(), L'.') != 3) {
        return false;
    }

    while (end != std::wstring::npos || start < ip.length()) {
        if (end == std::wstring::npos) {
            end = ip.length();
        }

        std::wstring octetStr = ip.substr(start, end - start);
        if (octetStr.empty() || octetStr.length() > 3) {
            return false; // Octet is empty or too long
        }

        for (wchar_t c : octetStr) {
            if (!iswdigit(c)) {
                return false; // Non-digit character found
            }
        }

        long octet = wcstol(octetStr.c_str(), nullptr, 10);
        if (octet < 0 || octet > 255) {
            return false; // Octet is out of valid range
        }

        numDots += (end != ip.length());
        start = end + 1;
        end = ip.find(L'.', start);
    }

    return true;
}

DWORD GetProcessIntegrityLevel(HANDLE tokenHandle) {
    DWORD integrityLevel = 0;
    DWORD dwLengthNeeded = 0;
    PTOKEN_MANDATORY_LABEL pTIL = NULL;
    if (!GetTokenInformation(tokenHandle, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded) && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(LPTR, dwLengthNeeded);
        if (pTIL) {
            if (GetTokenInformation(tokenHandle, TokenIntegrityLevel, pTIL, dwLengthNeeded, &dwLengthNeeded)) {
                DWORD subAuthorityCount = *GetSidSubAuthorityCount(pTIL->Label.Sid);
                integrityLevel = *GetSidSubAuthority(pTIL->Label.Sid, subAuthorityCount - 1);
            }
            LocalFree(pTIL);
        }
    }
    return integrityLevel;
}

bool IsAppRunningAsAdmin() {
    bool isAdmin = false;
    HANDLE tokenHandle = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
        // Get the size of the token's integrity level information.
        DWORD tokenInfoLength = 0;
        GetTokenInformation(tokenHandle, TokenIntegrityLevel, NULL, 0, &tokenInfoLength);
        TOKEN_MANDATORY_LABEL* tokenLabel = (TOKEN_MANDATORY_LABEL*)LocalAlloc(LPTR, tokenInfoLength);

        if (tokenLabel != NULL) {
            if (GetTokenInformation(tokenHandle, TokenIntegrityLevel, tokenLabel, tokenInfoLength, &tokenInfoLength)) {
                DWORD integrityLevel = *GetSidSubAuthority(tokenLabel->Label.Sid,
                    (DWORD)(UCHAR)(*GetSidSubAuthorityCount(tokenLabel->Label.Sid) - 1));

                // Security identifier (SID) that represents a high integrity level is S-1-16-12288.
                // We check if the current process's integrity level is high or higher (system).
                if (integrityLevel >= SECURITY_MANDATORY_HIGH_RID) {
                    isAdmin = true;
                }
            }
            LocalFree(tokenLabel);
        }
        CloseHandle(tokenHandle);
    }

    return isAdmin;
}

void CheckApplicationPrivileges() {
    HWND hwndFlightSim = FindWindow(target_window.c_str(), NULL);
    if (!hwndFlightSim) {
        is_flightsimulator_running = false;
        // Directly check if running as admin before deciding on permissions.
        has_permission = IsAppRunningAsAdmin();
        return;
    }
    else {
        is_flightsimulator_running = true; // Flight Simulator is running
    }

    DWORD targetProcessId;
    GetWindowThreadProcessId(hwndFlightSim, &targetProcessId);

    HANDLE targetProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, targetProcessId);
    if (!targetProcessHandle) {
        // Keep is_flightsimulator_running as true since the window was found
        // Final check for admin privileges if cannot open process.
        has_permission = IsAppRunningAsAdmin();
        return;
    }

    HANDLE targetTokenHandle;
    if (!OpenProcessToken(targetProcessHandle, TOKEN_QUERY, &targetTokenHandle)) {
        CloseHandle(targetProcessHandle);
        // Keep is_flightsimulator_running as true since the window was found
        // Final check for admin privileges if cannot open token.
        has_permission = IsAppRunningAsAdmin();
        return;
    }

    DWORD targetIntegrityLevel = GetProcessIntegrityLevel(targetTokenHandle);
    CloseHandle(targetTokenHandle);
    CloseHandle(targetProcessHandle);

    HANDLE currentProcessHandle = GetCurrentProcess();
    HANDLE currentTokenHandle;
    if (!OpenProcessToken(currentProcessHandle, TOKEN_QUERY, &currentTokenHandle)) {
        // Keep is_flightsimulator_running as true since the window was found
        // Final check for admin privileges if cannot open current process token.
        has_permission = IsAppRunningAsAdmin();
        return;
    }

    DWORD currentIntegrityLevel = GetProcessIntegrityLevel(currentTokenHandle);
    CloseHandle(currentTokenHandle);

    // Here, has_permission is true if the current process's integrity level is 
    // equal to or higher than the target's, OR if the app is running as admin.
    has_permission = (currentIntegrityLevel >= targetIntegrityLevel) || IsAppRunningAsAdmin();
}

HANDLE GetParentProcessHandle(DWORD& outParentPID) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32);
    DWORD currentPID = GetCurrentProcessId();
    HANDLE hParent = NULL;
    outParentPID = 0;

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == currentPID) {
                outParentPID = pe32.th32ParentProcessID;
                // Open a handle with necessary permissions
                hParent = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ParentProcessID);
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return hParent;
}

void MonitorParentProcessAsync(DWORD parentPID) {
    std::thread([parentPID]() {
        HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, parentPID);
        if (hParent == NULL) {
            wprintf(L"Failed to open parent process for monitoring.\n");
            return;
        }

        WaitForSingleObject(hParent, INFINITE);
        wprintf(L"Will now close the parent.\n");

        CloseHandle(hParent);

        cleanupServer();
        exit(0);

        // PostQuitMessage(0); // Example of signaling the main thread to quit

        // The above line will signal the main thread to quit the message loop so all the code after the loop will be executed and cleanup can be done

        }).detach(); // Detach the thread since we won't be joining it
}

std::wstring GetProcessName(HANDLE hProcess) {
    std::vector<wchar_t> processName(MAX_PATH);
    DWORD size = static_cast<DWORD>(processName.size());
    if (QueryFullProcessImageNameW(hProcess, 0, processName.data(), &size)) {
        std::wstring fullPath(processName.begin(), processName.begin() + size);
        size_t lastBackslashIndex = fullPath.find_last_of(L'\\');
        if (lastBackslashIndex != std::wstring::npos) {
            return fullPath.substr(lastBackslashIndex + 1);
        }
        return fullPath;
    }
    return L"Unknown";
}

void monitorParentProcess() {
    // Initialize the parent PID variable
    DWORD parentPID = 0;

    // Get the parent process handle and simultaneously retrieve the parent PID
    HANDLE hParent = GetParentProcessHandle(parentPID);
    std::wstring parentProcessName = GetProcessName(hParent);

    // Check if we got a valid handle and parent PID
    if (hParent && hParent != INVALID_HANDLE_VALUE && parentPID != 0) {
        // Close the handle as it's no longer needed; we just needed the PID for monitoring
        CloseHandle(hParent);

        if (parentProcessName.find(app_process) != std::wstring::npos) {
            // Monitor the parent process
            MonitorParentProcessAsync(parentPID);
            wprintf(L"Parent process is %s. Monitoring process.\n", app_process.c_str());
        }
        else {
            wprintf(L"Parent process is NOT %s.\n", app_process.c_str());
        }
    }
    else {
        // Handle the error or case when there is no parent process to monitor
        wprintf(L"No parent process detected.\n");
    }
}

bool isAlreadyRunning() {
    // Check if the application is already running
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"SendFSKeyMutex");
    if (hMutex == NULL) {
        // Handle error condition if necessary
        return false; // Or true, depending on how you want to handle mutex creation failure
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex); // Close the handle if we're not the first instance
        return true;
    }

    // No need to close the mutex handle here if this is the first instance.
    // The handle will be automatically closed when the application exits.
    // You can store hMutex globally if you want to explicitly release it on application exit,
    // but it's not strictly necessary.
    return false;
}

void MinimizeToTray(HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_TRAYICON;

    // Load the tray icon
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SENDFSKEY));
    if (!nid.hIcon) {
        MessageBox(NULL, L"Failed to load icon", L"Error", MB_ICONERROR);
    }

    // Load the balloon notification icon
#pragma warning(push)
#pragma warning(disable : 4302)
    nid.hBalloonIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SENDFSKEY), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
#pragma warning(pop)
    if (!nid.hBalloonIcon) {
        MessageBox(NULL, L"Failed to load balloon icon", L"Error", MB_ICONERROR);
    }

    // Set the tooltip text to the file description
    std::wstring FileDescription = GetSimpleVersionInfo(L"FileDescription");
    wcsncpy_s(nid.szTip, _countof(nid.szTip), FileDescription.c_str(), _countof(nid.szTip) - 1);

    // Set up balloon notification text
    nid.dwInfoFlags = NIIF_USER; // Use the user-defined icon for the balloon
    wcscpy_s(nid.szInfo, _countof(nid.szInfo), L"Application minimized to tray.");
    wcscpy_s(nid.szInfoTitle, _countof(nid.szInfoTitle), L"SendFSKey");

    // Add the icon to the system tray
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RestoreFromTray(HWND hWnd) {
    ShowWindow(hWnd, SW_SHOW);
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hWnd, POINT curPoint)
{
    // Create a menu or load it from resources
    HMENU hPopupMenu = CreatePopupMenu();
    if (!hPopupMenu) return;

    // Append menu items
    InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, L"Open");
    InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_CONSOLE, L"Toggle Console");

    InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Shutdown");

    // Add more items here

    // Set the default menu item (optional)
    SetMenuDefaultItem(hPopupMenu, ID_TRAY_OPEN, FALSE);

    // Display the menu
    TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hWnd, NULL);

    // Clean up
    DestroyMenu(hPopupMenu);
}

void UpdateMenuCheckMarks(HWND hwnd) {
    HMENU hMenu = GetMenu(hwnd); // Assuming hWnd is your main window's handle
    UINT checkState = (start_minimized == L"Yes") ? MF_CHECKED : MF_UNCHECKED;
    CheckMenuItem(hMenu, ID_OPTIONS_MINIMIZEONSTART, MF_BYCOMMAND | checkState);
}

std::wstring MarkdownToRtf(const std::string& markdown) {
    const size_t lineWidth = 150; // Hardcoded width limit for wrapping text
    // Define the colors we extracted
    std::wstring rtfColorTable = L"{\\colortbl ;"
        L"\\red109\\green75\\blue34;"    // Note color
        L"\\red22\\green27\\blue34;"     // Tip color
        L"\\red139\\green133\\blue106;"  // Important color
        L"\\red0\\green76\\blue71;"      // Warning color
        L"\\red22\\green27\\blue34;"     // Caution color (seems the same as Tip, you may need to adjust this)
        L"}\n";

    // Other RTF header content
    std::wstring rtfHeader = L"{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Segoe UI Variable;}}\n";

    // Start with the color table after the font table in the header
    std::wstring rtfContent = rtfHeader + rtfColorTable;

    std::istringstream iss(markdown);
    std::string line;
    while (std::getline(iss, line)) {
        // Check for headers
        if (line.substr(0, 2) == "# ") {
            rtfContent += L"\\fs28 \\b " + std::wstring(line.begin() + 2, line.end()) + L"\\b0\\par\n";
            continue;
        }
        else if (line.substr(0, 3) == "## ") {
            rtfContent += L"\\fs24 \\b " + std::wstring(line.begin() + 3, line.end()) + L"\\b0\\par\n";
            continue;
        }
        else if (line.substr(0, 4) == "### ") {
            rtfContent += L"\\fs22 \\b " + std::wstring(line.begin() + 4, line.end()) + L"\\b0\\par\n";
            continue;
        }

        // Now let's add the handling for the special prefixes
        std::wstring prefix;
        if (line.rfind("!NOTE", 0) == 0) {
            prefix = L"\\cf1 "; // Note color
            line = line.substr(5); // Strip the prefix
        }
        else if (line.rfind("!TIP", 0) == 0) {
            prefix = L"\\cf2 "; // Tip color
            line = line.substr(4); // Strip the prefix
        }
        else if (line.rfind("!IMPORTANT", 0) == 0) {
            prefix = L"\\cf3 "; // Important color
            line = line.substr(10); // Strip the prefix
        }
        else if (line.rfind("!WARNING", 0) == 0) {
            prefix = L"\\cf4 "; // Warning color
            line = line.substr(8); // Strip the prefix
        }
        else if (line.rfind("!CAUTION", 0) == 0) {
            prefix = L"\\cf5 "; // Caution color
            line = line.substr(8); // Strip the prefix
        }

        // Apply Markdown-to-RTF style conversions
        line = std::regex_replace(line, std::regex("\\*\\*\\*(.*?)\\*\\*\\*"), "{\\b\\i $1\\i0\\b0}");
        line = std::regex_replace(line, std::regex("\\*\\*(.*?)\\*\\*"), "{\\b $1\\b0}");
        line = std::regex_replace(line, std::regex("\\*(.*?)\\*"), "{\\i $1\\i0}");
        line = std::regex_replace(line, std::regex("~~(.*?)~~"), "{\\strike $1\\strike0}");
        std::wstring wLine(line.begin(), line.end());

        // If we have a prefix, add it to the line
        if (!prefix.empty()) {
            wLine = L"{" + prefix + wLine + L"\\cf0}";
        }

        // Wrap the processed line at the specified width
        size_t start = 0;
        while (start < wLine.length()) {
            size_t end = start + lineWidth < wLine.length() ? start + lineWidth : wLine.length();
            if (end < wLine.length()) {
                size_t lastSpace = wLine.rfind(L' ', end);
                if (lastSpace != std::wstring::npos && lastSpace > start) {
                    end = lastSpace;
                }
            }
            rtfContent += wLine.substr(start, end - start) + L"\\par\n";
            start = end + (start < wLine.length() && wLine[end] == L' ' ? 1 : 0);
        }
    }
    std::wstring rtfFooter = L"}";
    return rtfContent + rtfFooter;
}

std::string Utf16ToUtf8(const std::wstring& utf16Str)
{
    if (utf16Str.empty()) return std::string();

    // Determine the size of the buffer needed
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, utf16Str.c_str(), (int)utf16Str.size(), NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0) return std::string();

    std::string utf8Str(sizeNeeded, 0);
    // Perform the actual conversion
    int convertedLength = WideCharToMultiByte(CP_UTF8, 0, utf16Str.c_str(), (int)utf16Str.size(), &utf8Str[0], sizeNeeded, NULL, NULL);

    if (convertedLength != sizeNeeded) return std::string(); // Conversion failed

    return utf8Str;
}

void LoadTextFromResourceToEditControl(HWND hDlg, UINT nIDDlgItem) {

    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_TEXT1), RT_RCDATA);
    if (!hRes) {
        MessageBox(hDlg, L"Resource not found.", L"Error", MB_OK);
        return;
    }
    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) {
        MessageBox(hDlg, L"Failed to load resource.", L"Error", MB_OK);
        return;
    }

    // Resource loading code remains the same...
    LPCSTR lpData = static_cast<LPCSTR>(LockResource(hData));
    DWORD dwSize = SizeofResource(NULL, hRes);

    // Assuming the resource data is UTF-8 encoded Markdown text
    std::string markdownText(lpData, dwSize);

    // Convert Markdown to RTF
    std::wstring rtfContent = MarkdownToRtf(markdownText);

    // Example of a UTF-8 encoded RTF string (ensure the actual string is UTF-8 encoded)
    const char* utf8RtfTest = "{\\rtf1\\ansi This is a \\b bold\\b0 test.}";

    // Convert std::wstring (rtfContent) to a UTF-8 encoded std::string
    // std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    // std::string utf8RtfContent = converter.to_bytes(rtfContent);

    std::string utf8RtfContent = Utf16ToUtf8(rtfContent); // New

    HWND hRichEdit = GetDlgItem(hDlg, nIDDlgItem);

    // Prepare the Rich Edit control to accept UTF-8 encoded RTF content
    SETTEXTEX st = { ST_DEFAULT, CP_UTF8 };
    SendMessage(hRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)utf8RtfContent.c_str());
}