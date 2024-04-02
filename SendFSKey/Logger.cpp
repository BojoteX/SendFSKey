#include <windows.h>
#include "Globals.h"
#include "utilities.h"
#include "Logger.h"

// Logger class constructor
Logger::Logger(const std::wstring& logFileName) {
    std::wstring logFilePath = GetAppDataLocalSendFSKeyDir() + L"\\" + logFileName;
    outFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!outFile.is_open()) {
        throw std::runtime_error("Unable to open log file.");
    }
}

// Logger class destructor
Logger::~Logger() {
    if (outFile.is_open()) {
        outFile.close();
    }
}

// Log a message
void Logger::log(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mu);
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::wstringstream dateTime;
    std::tm nowTm = {}; // Initialize tm structure
    localtime_s(&nowTm, &nowTimeT); // Secure alternative to localtime
    dateTime << std::put_time(&nowTm, L"%Y-%m-%d %H:%M:%S");
    dateTime << L'.' << std::setfill(L'0') << std::setw(3) << nowMs.count();

    outFile << dateTime.str() << L" - " << message << std::endl;
}