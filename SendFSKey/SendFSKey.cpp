#include <windows.h>
#include <stdio.h>
#include "Globals.h"
#include "NetworkClient.h"

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);

// Debug console
bool DEBUG = 1; // Definition

wchar_t const* windowName = L"Microsoft Flight Simulator - Remote Key Sender";
wchar_t const* className = L"AceApp";

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

    if (!initializeWinsock()) {
        return -1; // Exit if Winsock initialization fails
    }

    // Establish connection at startup
    if (!establishConnection()) {
        if (DEBUG) printf("Failed to establish initial connection.\n");
        return -1;
    }

    if (bool(DEBUG)) {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    }

    WNDCLASS wc = { 0 };
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClass(&wc)) return -1;

    HWND hWnd = CreateWindowEx(0, className, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 800, 600, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return -1;

    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup before exit
    closeConnection();
    cleanupWinsock();

    return (int)msg.wParam; // Return the exit code
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_KEYDOWN:
        // printf("SENDING Key Down: %llu\n", static_cast<unsigned long long>(wp));
        sendKeyPress(static_cast<UINT>(wp));
        break;
    case WM_KEYUP:
        printf("Received: %llu\n", static_cast<unsigned long long>(wp));
        break;
    case WM_CHAR:
        wprintf(L"Char: %c\n", static_cast<wchar_t>(wp));
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}