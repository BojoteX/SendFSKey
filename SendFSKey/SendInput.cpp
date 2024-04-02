// SendInput.cpp
#include <Windows.h>
#include "Globals.h"
#include "Utilities.h"

// Change how we send the keys
bool USE_SCAN_CODE = 1; // 1 USES HARDWARE SCAN CODES - 0 USES VK KeyCodes (VK Codes have issues with keys like SHIFT so we never use it)

// Function to check if the specified window by class name is the foreground window
bool IsWindowInFocusByClassName(const char* MSFSclassName) {
    HWND hwndTarget = FindWindowA(MSFSclassName, NULL); // Window title is NULL since we're using class name
    return GetForegroundWindow() == hwndTarget;
}

// Function to bring the specified window by class name to the foreground if it's not already
bool BringWindowToForegroundByClassName(const wchar_t* MSFSclassName) {
    HWND hwndTarget = FindWindowW(NULL, MSFSclassName); // Use FindWindowW for wide characters, class name is now the second parameter
    if (hwndTarget == NULL) {
        // Window not found
        return false;
    }

    if (GetForegroundWindow() != hwndTarget) {
        // Window is not in focus, attempt to bring it to foreground
        return SetForegroundWindow(hwndTarget) != 0;
    }

    // Window was already in focus
    return true;
}

UINT ScanCodeRelease(UINT KeyCode) {
    UINT scanCode = MapVirtualKey(KeyCode, MAPVK_VK_TO_VSC);

    INPUT ip = { 0 };
    ip.type = INPUT_KEYBOARD;

    // Scan Code release
    ip.ki.wVk = scanCode; // Use the decimal value directly as the virtual key code.
    ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // Indicate key release

    // Send the KEY_UP
    SendInput(1, &ip, sizeof(INPUT));

    if (DEBUG) printf("[FORCED SCAN_CODE RELEASE] SendInput: KEY_UP sent: %u\n", KeyCode);

    return 1;
}

UINT KeyRelease(UINT KeyCode) {
    UINT scanCode = MapVirtualKey(KeyCode, MAPVK_VK_TO_VSC);

    INPUT ip = { 0 };
    ip.type = INPUT_KEYBOARD;

    // Key release
    ip.ki.wVk = KeyCode; // Use the decimal value directly as the virtual key code.
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // Indicate key release

    // Send the KEY_UP
    SendInput(1, &ip, sizeof(INPUT));

    if (DEBUG) printf("[FORCED VK_CODE RELEASE] SendInput: KEY_UP sent: %u\n", KeyCode);

    return 1;
}

UINT SendDOWNKeyWithScanCode(UINT virtualKeyCode) {
    UINT scanCode = MapVirtualKey(virtualKeyCode, MAPVK_VK_TO_VSC);
    INPUT ip = {};

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = scanCode;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;

    // Check if the key is an extended key
    if (virtualKeyCode == VK_RMENU || virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_INSERT) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &ip, sizeof(INPUT));
    printf("* [KEY_DOWN] SENT TO FLIGHT SIMULATOR: (%u)\n", virtualKeyCode);

    return 1;
}

UINT SendUPKeyWithScanCode(UINT virtualKeyCode) {
    UINT scanCode = MapVirtualKey(virtualKeyCode, MAPVK_VK_TO_VSC);
    INPUT ip = {};

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = scanCode;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;

    // Check if the key is an extended key
    if (virtualKeyCode == VK_RMENU || virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_INSERT) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    // If it's a key release, add KEYEVENTF_KEYUP
    ip.ki.dwFlags |= KEYEVENTF_KEYUP;

    SendInput(1, &ip, sizeof(INPUT));

    printf("* [KEY_UP] SENT TO FLIGHT SIMULATOR: (%u)\n", virtualKeyCode);

    return 1;
}

UINT SendDOWNKeyWithoutScanCode(UINT virtualKeyCode) {
    INPUT ip = {};

    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = virtualKeyCode; // Use the decimal value directly as the virtual key code.
    ip.ki.dwFlags = 0;

    // Check if the key is an extended key
    if (virtualKeyCode == VK_RMENU || virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_INSERT) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &ip, sizeof(INPUT));
    printf("[KEY_DOWN] SendInput VK_MODE (%u) \n", virtualKeyCode);

    return 1;
}

UINT SendUPKeyWithoutScanCode(UINT virtualKeyCode) {
    INPUT ip = {};

    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = virtualKeyCode; // Use the decimal value directly as the virtual key code.
    ip.ki.dwFlags = KEYEVENTF_KEYUP;

    // Check if the key is an extended key
    if (virtualKeyCode == VK_RMENU || virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_INSERT) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &ip, sizeof(INPUT));
    printf("[KEY_UP] SendInput VK_MODE (%u) \n", virtualKeyCode);

    return 1;
}

UINT ServerKeyPressDOWN(UINT KeyCode) {

    // We only want to send the key press if the MSFS window is in focus, if not, we bring it to focus
    BringWindowToForegroundByClassName(target_window.c_str());

    /*

    // Log the KEY_DOWN on the server side
    std::wstringstream logStream;
    logStream << L"Key pressed: " << KeyCode;
    Logger::GetInstance()->log(logStream.str());

    */

    if (USE_SCAN_CODE) {
        SendDOWNKeyWithScanCode(KeyCode);
    }
    else {
        SendDOWNKeyWithoutScanCode(KeyCode);
    }
    return 1;
}

UINT ServerKeyPressUP(UINT KeyCode) {

    if (USE_SCAN_CODE) {
        SendUPKeyWithScanCode(KeyCode);
    }
    else {
        SendUPKeyWithoutScanCode(KeyCode);
    }
    
    if (DEBUG) {
        // Slight pause after the key up to see what got stuck
        Sleep(25);
        for (int key = 0; key <= 0xFF; ++key) {
            if (GetAsyncKeyState(key) & 0x8000) {
                printf("[DEBUG] The following key is still in a KEY_DOWN state: %u\n", key);
                KeyRelease(key);
                ScanCodeRelease(key);
            }
        }
    }

    return 1;
}