#pragma once

#include "Globals.h"

bool initializeWinsock();
void cleanupWinsock();
bool establishConnection();
bool establishConnectionAsync();
void closeClientConnection();