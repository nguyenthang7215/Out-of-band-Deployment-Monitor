#pragma once

#include <string> 
#include <mutex>
#include <fstream> 

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public: 
    static Logger& instance();
    // Tranh copy
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    ~Logger();
    
    void init(const std::string& log_file_path, LogLevel min_level = LogLevel::INFO);
    void log(LogLevel level, const std::string &msg);

private: 
    Logger() = default;

    std::ofstream log_file;
    std::mutex mtx;
    LogLevel min_level = LogLevel::INFO;

    std::string level_to_string(LogLevel level) const; 
};

#define LOG_DEBUG(msg) Logger::instance().log(LogLevel::DEBUG, msg)
#define LOG_INFO(msg) Logger::instance().log(LogLevel::INFO, msg)
#define LOG_WARN(msg) Logger::instance().log(LogLevel::WARN, msg)
#define LOG_ERROR(msg) Logger::instance().log(LogLevel::ERROR, msg)