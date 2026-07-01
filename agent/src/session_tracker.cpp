#include "session_tracker.h"

void SessionTracker::process_line(const std::string& line) {
    if (line.find("type=USER_LOGIN") != std::string::npos || line.find("type=USER_START") != std::string::npos) {
        parse_login_line(line);
    }
}

std::string SessionTracker::get_source_ip(int session_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = session_map.find(session_id);
    if (it != session_map.end()) {
        return it->second;
    }
    return "";
}

void SessionTracker::parse_login_line(const std::string& line) {
    std::string res = extract_field(line, "res="); // result

    if (res != "success") {
        return;
    }

    std::string ses_str = extract_field(line, "ses=");
    if (ses_str.empty()) {
        return;
    }

    // Xu ly truong hop session ao (cron job, internal process) = 4294967295
    unsigned long ses_raw = 0;
    try {
        ses_raw = std::stoul(ses_str);
    } catch (...) {
        return;
    }

    if (ses_raw == 4294967295UL) {
        return;
    }
    
    int session_id = static_cast<int>(ses_raw);

    std::string addr = extract_field(line, "addr=");
    if (addr.empty()) {
        return;
    }

    if (!is_valid_ip(addr)) {
        return;
    }

    std::lock_guard<std::mutex> lock(mtx);
    session_map[session_id] = addr;
}

bool SessionTracker::is_valid_ip(const std::string& addr) const {
    // IP local hoac IP rong
    if (addr == "?" || addr == "127.0.0.1" || addr == "::1") {
        return false;
    }
    return true;
}

std::string SessionTracker::extract_field(const std::string& line, const std::string& key) const {
    std::size_t start_pos = line.find(key);
    if (start_pos == std::string::npos) {
        return "";
    }

    start_pos += key.length(); // Nhay den vi tri sau dau =
    std::string value;

    if (start_pos < line.length() && line[start_pos] == '"') {
        start_pos++; // Nhay den vi tri sau dau "

        std::size_t end_pos = line.find('"', start_pos);
        if (end_pos == std::string::npos) {
            value = line.substr(start_pos);
        } else {
            value = line.substr(start_pos, end_pos - start_pos); // Lay chuoi ket thuc giua 2 dau "
        }
    } 
    // TH binh thuong (khong co ngoac kep), ket thuc bang dau cach
    else {
        std::size_t end_pos = line.find(' ', start_pos);
        if (end_pos == std::string::npos) {
            value = line.substr(start_pos);
        } else {
            value = line.substr(start_pos, end_pos - start_pos);
        }
        
        while (!value.empty() && (value.back() == '\'' || value.back() == ')' || value.back() == '"' || value.back() == ',')) {
            value.pop_back();
        }
    }

    while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
        value.pop_back();
    }
    return value;
}