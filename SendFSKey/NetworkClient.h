#pragma once

bool initializeWinsock();
void sendKeyPress(UINT keyCode);
void cleanupWinsock();
bool establishConnection();
void closeClientConnection();
