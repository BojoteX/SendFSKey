#pragma once

#include <shlobj.h>
#include <fstream>
#include <TlHelp32.h>
#include "Globals.h"

// Settings
std::wstring mode = L"Server";
std::wstring serverIP = L"0.0.0.0";
int port = 8028;

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

std::wstring FormatForDisplay(const std::string& data) {
    // Convert std::string (assumed to be UTF-8 or ASCII) to std::wstring
    std::wstring wideData(data.begin(), data.end());

    // Format the message
    std::wstring formattedMessage = L"Received data: " + wideData + L"\n";
    return formattedMessage;
}

UINT getKey(UINT keyCodeNum) {

    if (keyCodeNum == VK_SHIFT) {
        if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
            keyCodeNum = 160;
        }
        else if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) {
            keyCodeNum = 161;
        }
    }

    if (keyCodeNum == VK_CONTROL) {
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
            keyCodeNum = 162;
        }
        else if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) {
            keyCodeNum = 163;
        }
    }

    if (keyCodeNum == VK_MENU) {
        if (GetAsyncKeyState(VK_LMENU) & 0x8000) {
            keyCodeNum = 164;
        }
        else if (GetAsyncKeyState(VK_RMENU) & 0x8000) {
            keyCodeNum = 165;
        }
    }

    return keyCodeNum;
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

                // Cleanup
                shutdownServer();

                // Exit application
                PostQuitMessage(0); // Or use a graceful shutdown method suitable for your application
                break;
            }
        }
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
            return;
        }

        // Close the current instance
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
        if (!DeleteFileW(iniPath.c_str())) {
            DWORD error = GetLastError();
            MessageBox(NULL, L"Failed to delete the configuration file.", L"Error", MB_ICONERROR | MB_OK);
            return; // Exit the function if unable to delete the file
        }
    }

    // Restart the application
    RestartApplication();
}