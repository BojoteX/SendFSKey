#pragma once

#include "Globals.h"

bool initializeWinsock();
void sendKeyPress(UINT keyCode, bool isKeyDown);
void cleanupWinsock();
bool establishConnection();
void closeClientConnection();