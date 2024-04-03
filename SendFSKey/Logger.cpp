// Logger.cpp
#include <windows.h>
#include "Globals.h"
#include "utilities.h"
#include "Logger.h"

Logger* Logger::instance = nullptr;

Logger* Logger::GetInstance(const std::wstring& logFile) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr) {
        instance = new Logger(logFile);
    }
    return instance;
}

Logger::Logger(const std::wstring& logFileName) {
    std::wstring logFilePath = GetAppDataLocalSendFSKeyDir() + L"\\" + logFileName;
    outFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!outFile.is_open()) {
        throw std::runtime_error("Unable to open log file.");
    }
}

Logger::~Logger() {
    if (outFile.is_open()) {
        outFile.close();
    }
}

void Logger::log(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mu);

    // Get current time with high-resolution
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    auto nowMs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    std::wstringstream dateTime;
    std::tm nowTm = {};
    localtime_s(&nowTm, &nowTimeT);

    // Format the date and time up to seconds
    dateTime << std::put_time(&nowTm, L"%Y-%m-%d %I:%M:%S"); // Added %Y-%m-%d for date

    // Add microseconds
    dateTime << L'.' << std::setfill(L'0') << std::setw(7) << nowMs.count();

    // Add AM/PM part
    dateTime << std::put_time(&nowTm, L" %p");

    outFile << dateTime.str() << L" - " << message << std::endl;
}

/*
void Logger::log(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mu);
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::wstringstream dateTime;
    std::tm nowTm = {};
    localtime_s(&nowTm, &nowTimeT);
    dateTime << std::put_time(&nowTm, L"%Y-%m-%d %H:%M:%S");
    dateTime << L'.' << std::setfill(L'0') << std::setw(3) << nowMs.count();

    outFile << dateTime.str() << L" - " << message << std::endl;
}


void Logger::log(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mu);

    // Get current time with high-resolution
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    auto nowMs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    std::wstringstream dateTime;
    std::tm nowTm = {};
    localtime_s(&nowTm, &nowTimeT);

    // Format the time up to seconds
    dateTime << std::put_time(&nowTm, L"%I:%M:%S"); // Use %I for 12-hour clock format

    // Add microseconds
    dateTime << L'.' << std::setfill(L'0') << std::setw(7) << nowMs.count();

    // Add AM/PM part
    dateTime << std::put_time(&nowTm, L" %p");

    outFile << dateTime.str() << L" - " << message << std::endl;
}

*/