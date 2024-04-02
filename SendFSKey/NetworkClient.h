#pragma once

// Forward declarations
bool initializeWinsock();
bool establishConnection();

// Client functions
void closeClientConnection();
void cleanupWinsock();
void sendKeyPress(UINT keyCode, bool isKeyDown);
void clientConnectionThread();