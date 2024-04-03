#pragma once

#include <fstream>
#include <sstream>
#include <iomanip>

class Logger {
public:
    static Logger* GetInstance(const std::wstring& logFile = L"SendFSKey.log");
    void log(const std::wstring& message);
    ~Logger();

private:
    Logger(const std::wstring& logFile); // Private constructor to prevent direct instantiation.
    static Logger* instance; // Static instance pointer for singleton pattern.
    std::wofstream outFile;
    std::mutex mu;

    // Deleted copy constructor and assignment operator to prevent copies.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};