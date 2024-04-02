#pragma once

#include <shlobj.h>

// Tell the linker to include the Version library
#pragma comment(lib, "Version.lib")

#define WM_TRAYICON (WM_USER + 1)

// Forward declarations
bool IniFileExists(const std::string& filename);
bool IsValidIPv4(const std::wstring& ip);
bool isAlreadyRunning();

std::wstring GetAppDataLocalSendFSKeyDir();
std::wstring GetSimpleVersionInfo(const std::wstring& infoType);

void WriteSettingsToIniFile(const std::wstring& mode, const std::wstring& ip);
void getKey(UINT keyCodeNum, bool isSystemKey, bool isKeyDown);
void ToggleConsoleVisibility(const std::wstring& title);
void DeleteIniFileAndRestart();
void CheckApplicationPrivileges();
void monitorParentProcess();
void MinimizeToTray(HWND hWnd);
void UpdateMenuCheckMarks(HWND hwnd);
void LoadTextFromResourceToEditControl(HWND hDlg, UINT nIDDlgItem);
void ShowContextMenu(HWND hWnd, POINT curPoint);