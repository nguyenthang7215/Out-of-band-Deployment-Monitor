#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

class SessionTracker {
public:
    void process_line(const std::string& line); // Loc lay dong USER_LOGIN/USER_START
    std::string get_source_ip(int session_id) const;

private:
    void parse_login_line(const std::string& line); // Parse lay ses va addr
    std::string extract_field(const std::string& line, const std::string& key) const;
    bool is_valid_ip(const std::string& addr) const;
    std::unordered_map<int, std::string> session_map; // ses -> IP
    mutable std::mutex mtx;
};
