#pragma once

#include <iostream>
#include <fstream>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

std::wofstream logFile;

// Settings
std::wstring mode;
std::wstring serverIP;
int port = 8028;

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

std::wstring FormatForDisplay(const std::string& data) {
    // Convert std::string (assumed to be UTF-8 or ASCII) to std::wstring
    std::wstring wideData(data.begin(), data.end());

    // Format the message
    std::wstring formattedMessage = L"Received data: " + wideData + L"\n";
    return formattedMessage;
}