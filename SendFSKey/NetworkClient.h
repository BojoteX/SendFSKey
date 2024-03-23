#pragma once

bool initializeWinsock();
void sendKeyPress(UINT keyCode, bool isKeyDown);
void cleanupWinsock();
bool establishConnection();
void closeClientConnection();
