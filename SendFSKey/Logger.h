/*
#pragma once

#include <mutex>
#include <fstream>
#include <string> // Include for std::wstring
#include <sstream>
#include <iomanip>


class Logger {
public:
    static Logger* GetInstance(const std::wstring& logFile = L"log.txt");
    ~Logger(); // Destructor declaration

    void log(const std::wstring& message);

private:
    std::wofstream outFile;
    std::mutex mu;
    Logger(const std::wstring& logFile); // Private constructor

    // Deleted copy constructor and copy assignment operator for singleton pattern
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Helper method to format the current time.
    std::wstring GetCurrentTimeFormatted();
};
*/