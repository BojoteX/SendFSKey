#pragma once

// Forward declarations

bool isServerUp();

void cleanupServer();
void serverStartThread();

std::wstring getServerIPAddress();

UINT ServerKeyPressDOWN(UINT KeyCode); // Used by server to send key presses
UINT ServerKeyPressUP(UINT KeyCode); // Used by server to send key releases