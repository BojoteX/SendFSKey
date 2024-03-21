#include <windows.h>
#include <stdio.h>

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND, UINT, WPARAM, LPARAM);

HWND hEdit;
HBRUSH hbrBackground; // Handle to the background brush
WNDPROC wpOriginalEditProc; // Original edit control window procedure

wchar_t const* className = L"AceApp";

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(255, 255, 255)); // White text
        SetBkColor(hdcEdit, RGB(0, 0, 0)); // Black background
        return (LRESULT)hbrBackground;
    }
    }
    return CallWindowProc(wpOriginalEditProc, hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    AllocConsole();

    // Secure version of freopen
    FILE* fp;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    WNDCLASS wc = { 0 };

    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClass(&wc)) return -1;

    HWND hWnd = CreateWindowEx(
        0,
        className,
        L"Microsoft Flight Simulator - 1.37.8.0",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100,
        100,
        800,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd) return -1;

    hEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0,
        0, 0,
        hWnd,
        (HMENU)1,
        hInstance,
        nullptr);

    if (!hEdit) return -1;

    wpOriginalEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
    hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Black background

    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(hbrBackground); // Clean up the brush
    return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE: {
        int width = LOWORD(lp);
        int height = HIWORD(lp);
        int margin = 10;

        MoveWindow(hEdit, margin, margin, width - margin * 2, height - margin * 2, TRUE);
        break;
    }
    case WM_KEYDOWN:
        printf("Key Down: %llu\n", static_cast<unsigned long long>(wp)); // For 64-bit compatibility
        break;
    case WM_KEYUP:
        printf("Key Up: %llu\n", static_cast<unsigned long long>(wp)); // For 64-bit compatibility
        break;
    case WM_CHAR: {
        wchar_t charPressed = (wchar_t)wp;
        wprintf(L"Char: %c\n", charPressed); // Log the character
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}