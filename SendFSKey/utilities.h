#pragma once

#include <shlobj.h>
#include <fstream>
#include <TlHelp32.h>
#include <unordered_map>
#include "Globals.h"

// Settings
std::wstring mode = L"Server";
std::wstring serverIPconf = L"0.0.0.0";
int port = 8028;

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
    if (!WritePrivateProfileStringW(L"Settings", L"Port", std::to_wstring(port).c_str(), iniPath.c_str())) {
        DWORD error = GetLastError();
    }
}

std::wstring FormatForDisplay(const std::string& data) {
    // Convert std::string (assumed to be UTF-8 or ASCII) to std::wstring
    std::wstring wideData(data.begin(), data.end());

    // Format the message
    std::wstring formattedMessage = L"Received data: " + wideData + L"\n";
    return formattedMessage;
}

void AppendTextToConsole(HWND hStatic, const std::wstring& text) {
    // Allocate new memory for the text to be sent to the window procedure
    std::wstring* pText = new std::wstring(text);

    // Post the custom message along with the pointer to the text
    printf("Posting message to GUI (console)\n");
    PostMessage(hStatic, WM_APPEND_TEXT_TO_CONSOLE, reinterpret_cast<WPARAM>(pText), 0);
}

void getKey(UINT keyCodeNum, bool isSystemKey, bool isKeyDown) {

    if (isKeyDown) {
        printf("\n* [KEY_DOWN] DETECTED from Client Keyboard: (%d)", keyCodeNum);
    }
    else {
        printf("* [KEY_UP] DETECTED from Client Keyboard: (%d)", keyCodeNum);
    }

    if (isSystemKey)
        printf(" as a SYSTEM key ");
    else {
		printf(" as a STANDARD key ");
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

bool isFlightSimulatorRunning() {
    const wchar_t* flightSimulatorExe = L"FlightSimulator.exe";
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, flightSimulatorExe) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return (found);
}

void MonitorFlightSimulatorProcess() {
    // Flight Simulator is running, start monitoring
    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!isFlightSimulatorRunning()) {
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
    if (consoleWindow == NULL) {
        // No console attached, try to create one
        if (AllocConsole()) {
            // Set console title using the passed title argument
            SetConsoleTitleW(title.c_str());
            // Redirect std input/output streams to the new console
            freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
            consoleWindow = GetConsoleWindow();
            ShowWindow(consoleWindow, SW_SHOW); // Ensure the new console window is shown
        }
    }
    else {
        // Toggle visibility based on current status
        bool isVisible = ::IsWindowVisible(consoleWindow) != FALSE;
        ShowWindow(consoleWindow, isVisible ? SW_HIDE : SW_SHOW);
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