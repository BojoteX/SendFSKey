#pragma once

#include <shlobj.h>
#include <fstream>
#include <TlHelp32.h>

std::atomic<bool> isFlightSimulatorRunning(false);

// Settings
std::wstring mode;
std::wstring serverIP;
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

void MonitorFlightSimulatorProcess() {
    const wchar_t* flightSimulatorExe = L"FlightSimulator.exe";

    while (true) {
        bool found = false;
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (Process32First(snapshot, &entry)) {
            do {
                if (!_wcsicmp(entry.szExeFile, flightSimulatorExe)) {
                    found = true;
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);

        isFlightSimulatorRunning = found;

        if (!found) {
            // Flight Simulator is not running; initiate shutdown procedure
            MessageBoxA(NULL, "Flight Simulator is not running, will now exit.", "Error", MB_ICONERROR);

            // Cleanup
            shutdownServer();

            exit(0); // or a graceful shutdown method for your application
        }

        // Sleep for a bit before checking again
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}