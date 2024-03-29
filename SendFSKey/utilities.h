#pragma once

#include <shlobj.h>
#include <fstream>
#include <TlHelp32.h>
#include <unordered_map>
#include <shellapi.h>
#include "Globals.h"

// Tell the linker to include the Version library
#pragma comment(lib, "Version.lib")

// Global scope or within a class/structure as needed
NOTIFYICONDATA nid = {}; // Zero-initialize the structure

// Debug mode (.ini file will override this)
bool DEBUG = FALSE;

// .ini Settings defaults
int port = 8028; // Default port can be changed in the INI file
std::wstring mode = L"Server";
std::wstring serverIPconf = L"0.0.0.0";
std::wstring target_window = L"AceApp";
std::wstring app_process = L"FlightSimulator.exe";
std::wstring use_queuing = L"No";

// .ini file settings definitions only
bool queueKeys;
int maxQueueSize; // Maximum number of keys to queue, its hardcoded to 4 for now
std::wstring consoleVisibility;

// Global variable to check if the application has permission to send keys and check if the process is running
bool has_permission = false;
bool is_flightsimulator_running = false;

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
    if (!WritePrivateProfileStringW(L"Settings", L"TargetWindow", target_window.c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
    if (!WritePrivateProfileStringW(L"Settings", L"AppProcess", app_process.c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
    if (!WritePrivateProfileStringW(L"Settings", L"UseQueing", use_queuing.c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
    if (!WritePrivateProfileStringW(L"Settings", L"Port", std::to_wstring(port).c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
    if (!WritePrivateProfileStringW(L"Settings", L"Debug", std::to_wstring(DEBUG).c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
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

void MonitorFlightSimulatorProcess() {
    // Flight Simulator is running, start monitoring
    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!is_flightsimulator_running) {
                // Flight Simulator is not running; initiate shutdown procedure
                MessageBoxA(NULL, "Flight Simulator is NOT running, will now exit.", "Error", MB_ICONERROR);
                break;
            }
        }
        // Exit application
        shutdownServer();
        ExitProcess(1); // Terminate application if unable to reconnect after max attempts
    }).detach(); // Detach the thread to allow it to run independently
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

            if(isClientMode)
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
            printf("Failed to restart the application\n");
            return;
        }

        if (isClientMode) {
            closeClientConnection(); // For client
            wprintf(L"Closing client connection\n");
        }
        else {
            // For server mode, insert cleanup operations here
            wprintf(L"Closing server connection\n");
            cleanupServer();
        }
        wprintf(L"Winsock cleanup and exiting program\n");
        cleanupWinsock();

        // Exit the current instance of the application gracefully after cleanup
        exit(0);
    }
    else {
        MessageBox(NULL, L"Failed to obtain application path.", L"Error", MB_ICONERROR | MB_OK);
        wprintf(L"Failed to obtain application path\n");
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

void CheckApplicationPrivileges() {
    HWND hwndFlightSim = FindWindow(target_window.c_str(), NULL);
    if (!hwndFlightSim) {
        has_permission = false;
        is_flightsimulator_running = false;
        return;
    }
    else {
        is_flightsimulator_running = true; // Flight Simulator is running
    }

    DWORD targetProcessId;
    GetWindowThreadProcessId(hwndFlightSim, &targetProcessId);

    HANDLE targetProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, targetProcessId);
    if (!targetProcessHandle) {
        has_permission = false;
        // Keep is_flightsimulator_running as true since the window was found
        return;
    }

    HANDLE targetTokenHandle;
    if (!OpenProcessToken(targetProcessHandle, TOKEN_QUERY, &targetTokenHandle)) {
        CloseHandle(targetProcessHandle);
        has_permission = false;
        // Keep is_flightsimulator_running as true since the window was found
        return;
    }

    DWORD targetIntegrityLevel = GetProcessIntegrityLevel(targetTokenHandle);
    CloseHandle(targetTokenHandle);
    CloseHandle(targetProcessHandle);

    HANDLE currentProcessHandle = GetCurrentProcess();
    HANDLE currentTokenHandle;
    if (!OpenProcessToken(currentProcessHandle, TOKEN_QUERY, &currentTokenHandle)) {
        has_permission = false;
        // Keep is_flightsimulator_running as true since the window was found
        return;
    }

    DWORD currentIntegrityLevel = GetProcessIntegrityLevel(currentTokenHandle);
    CloseHandle(currentTokenHandle);

    has_permission = currentIntegrityLevel >= targetIntegrityLevel;
}

void MinimizeToTray(HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(g_hInst_client, MAKEINTRESOURCE(IDI_SENDFSKEY));
    std::wstring FileDescription = GetSimpleVersionInfo(L"FileDescription");
    wcscpy_s(nid.szTip, _countof(nid.szTip), FileDescription.c_str());
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RestoreFromTray(HWND hWnd) {
    ShowWindow(hWnd, SW_SHOW);
    Shell_NotifyIcon(NIM_DELETE, &nid);
}