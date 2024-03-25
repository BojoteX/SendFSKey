// SendInput.cpp
#include <set>
#include <Windows.h>

// Change how we send the keys
bool USE_SCAN_CODE = 1; // 1 USES HARDWARE SCAN CODES - 0 USES VK KeyCodes
int EXTENDED_DEBUG = 0;

UINT ScanCodeRelease(UINT KeyCode) {
    UINT scanCode = MapVirtualKey(KeyCode, MAPVK_VK_TO_VSC);

    INPUT ip = { 0 };
    ip.type = INPUT_KEYBOARD;

    // Scan Code release
    ip.ki.wVk = scanCode; // Use the decimal value directly as the virtual key code.
    ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // Indicate key release

    // Send the KEY_UP
    SendInput(1, &ip, sizeof(INPUT));

    if (EXTENDED_DEBUG) printf("[FORCED SCAN CODE RELEASE] SendInput: KEY_UP sent: %u\n", KeyCode);

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

    if (EXTENDED_DEBUG) printf("[FORCED KEY RELEASE] SendInput: KEY_UP sent: %u\n", KeyCode);

    return 1;
}

UINT SendKeyWithScanCode(UINT virtualKeyCode, BOOL isKeyDown) {
    UINT scanCode = MapVirtualKey(virtualKeyCode, MAPVK_VK_TO_VSC);
    INPUT ip = {};

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = scanCode;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;

    // Check if the key is an extended key
    if (virtualKeyCode == VK_RMENU || virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_INSERT) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    if (!isKeyDown) {
        // If it's a key release, add KEYEVENTF_KEYUP
        ip.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    SendInput(1, &ip, sizeof(INPUT));

    if (isKeyDown)
        if (EXTENDED_DEBUG) printf("[SCANCODE] SendInput: KEY_DOWN sent: %u\n", virtualKeyCode);
    else
        if (EXTENDED_DEBUG) printf("[SCANCODE] SendInput: KEY_UP sent: %u\n", virtualKeyCode);

    return 1;
}

UINT SendKeyWithoutScanCode(UINT virtualKeyCode, BOOL isKeyDown) {
    INPUT ip = {};

    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = virtualKeyCode; // Use the decimal value directly as the virtual key code.

    /*
    // Check if the key is an extended key
    if (virtualKeyCode == VK_RMENU || virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_INSERT) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    */

    if (!isKeyDown) {
        // If it's a key release, add KEYEVENTF_KEYUP
        ip.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    SendInput(1, &ip, sizeof(INPUT));

    if (isKeyDown)
        if (EXTENDED_DEBUG) printf("[KEYCODE] SendInput: KEY_DOWN sent: %u\n", virtualKeyCode);
        else
            if (EXTENDED_DEBUG) printf("[KEYCODE] SendInput: KEY_UP sent: %u\n", virtualKeyCode);

    return 1;
}

UINT SendKeyPressDOWN(UINT KeyCode) {

    if (USE_SCAN_CODE) {
        SendKeyWithScanCode(KeyCode, 1); // 1 Means KEY_DOWN
    }
    else {
        INPUT ip = { 0 };
        ip.type = INPUT_KEYBOARD;
        ip.ki.wVk = KeyCode; // Use the decimal value directly as the virtual key code.

        /*
        // Check if the key is an extended key
        if (KeyCode == VK_RMENU || KeyCode == VK_RCONTROL || KeyCode == VK_INSERT) {
            ip.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
        }
        */

        // Send the KEY_DOWN
        SendInput(1, &ip, sizeof(INPUT));
        printf("SendInput: KEY_DOWN sent: %u\n", KeyCode);
    }
    return 1;
}

UINT SendKeyPressUP(UINT KeyCode) {

    if (EXTENDED_DEBUG) printf("[SWITCH CASE] Received: %u\n", KeyCode);

    // Assuming KeyCode is an array or vector that can hold multiple key codes.
    std::set<UINT> keyCodes;

    int found = 0;
    // Populate keyCodes based on the current state of SHIFT, CONTROL, and ALT keys
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
        keyCodes.insert(VK_LSHIFT);
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Added to array: %u\n", VK_LSHIFT);
        found = 1;
    }
    if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) {
        keyCodes.insert(VK_RSHIFT);
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Added to array: %u\n", VK_RSHIFT);
        found = 1;
    }
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
        keyCodes.insert(VK_LCONTROL);
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Added to array: %u\n", VK_LCONTROL);
        found = 1;
    }
    if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) {
        keyCodes.insert(VK_RCONTROL);
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Added to array: %u\n", VK_RCONTROL);
        found = 1;
    }
    if (GetAsyncKeyState(VK_LMENU) & 0x8000) {
        keyCodes.insert(VK_LMENU);
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Added to array: %u\n", VK_LMENU);
        found = 1;
    }
    if (GetAsyncKeyState(VK_RMENU) & 0x8000) {
        keyCodes.insert(VK_RMENU);
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Added to array: %u\n", VK_RMENU);
        found = 1;
    }

    // This is just a precaution
    if (found) {
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] Iterated the array for KeyReleases above\n");

        if (USE_SCAN_CODE) {
            for (UINT keyCode : keyCodes) {
                SendKeyWithScanCode(keyCode, FALSE); // FALSE for key release
            }
        }
        else {
            for (UINT keyCode : keyCodes) {
                SendKeyWithoutScanCode(keyCode, FALSE); // FALSE for key release
            }
        }
    }
    else {
        if (EXTENDED_DEBUG) printf("[SWITCH CASE] No modification was needed: %u\n", KeyCode);
    }

    if (USE_SCAN_CODE) {
        SendKeyWithScanCode(KeyCode, FALSE); // 1 Means KEY_DOWN
    }
    else {
        SendKeyWithoutScanCode(KeyCode, FALSE); // FALSE for key release
    }

    if (EXTENDED_DEBUG) {
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