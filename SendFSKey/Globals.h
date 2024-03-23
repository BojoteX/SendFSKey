#pragma once

#include <string>
#include <atomic>

extern std::atomic<bool> serverRunning;
extern std::wstring mode;
extern std::wstring serverIP;
extern int port;
extern HWND hEdit;

// To be abble to output to our window with the proper format
std::wstring FormatForDisplay(const std::string& data);
void AppendTextToConsole(HWND, const wchar_t* text);
