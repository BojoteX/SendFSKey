#pragma once

bool initializeWinsock();
void closeClientConnection();
void cleanupWinsock();
void sendKeyPress(UINT keyCode, bool isKeyDown);
bool establishConnection();
void clientConnectionThread();