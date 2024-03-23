// SendInput.cpp
#include <Windows.h>

UINT SendKeyPressDOWN(UINT KeyCode) {
    INPUT ip = { 0 };
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = KeyCode; // Use the decimal value directly as the virtual key code.

    // Key press
    ip.ki.dwFlags = 0; // 0 for key press
    return SendInput(1, &ip, sizeof(INPUT));
}

UINT SendKeyPressUP(UINT KeyCode) {
    INPUT ip = { 0 };
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = KeyCode; // Use the decimal value directly as the virtual key code.

    // Key press
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // Indicate key release
    return SendInput(1, &ip, sizeof(INPUT));
}