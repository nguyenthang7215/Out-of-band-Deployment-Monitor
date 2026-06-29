#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if(log_file.is_open()) {
        log_file.close();
    }
}

void Logger::init(const std::string& log_file_path, LogLevel min_level) { 
    this->min_level = min_level; // Cap nhat lai min_level
    log_file.open(log_file_path, std::ios::app);

    if(!log_file.is_open()) {
        std::cerr << "Không mở được file log: " << log_file_path << std::endl;
    }
}

void Logger::log(LogLevel level, const std::string &msg) {
    std::lock_guard<std::mutex> lock(mtx);

    if(level < min_level) { 
        return;
    }
    // Ghi theo dang: [formatted_time] [level] msg
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream formatted_time;
    formatted_time << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    std::string log_line = "[" + formatted_time.str() + "] [" + level_to_string(level) + "] " + msg;

    if(log_file.is_open()) { 
        log_file << log_line << std::endl;
    }

    std::cout << log_line << std::endl; // Cho debug (khong qua can thiet)
}

std::string Logger::level_to_string(LogLevel level) const {
    switch(level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}