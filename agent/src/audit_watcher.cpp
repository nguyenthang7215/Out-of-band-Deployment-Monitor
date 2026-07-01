#include "audit_watcher.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <pwd.h>
#include <sys/types.h>

AuditWatcher::AuditWatcher(SessionTracker& tracker) : session_tracker(tracker) {}

AuditWatcher::~AuditWatcher() {
    stop();
}

void AuditWatcher::set_callback(AuditEventCallback cb) {
    callback = cb;
}

void AuditWatcher::start(const std::string& log_file_path) {
    if (running) return;
    running = true;
    watcher_thread = std::thread(&AuditWatcher::tail_loop, this, log_file_path);
}

void AuditWatcher::stop() {
    if (running) {
        running = false;
        if (watcher_thread.joinable()) {
            watcher_thread.join();
        }
    }
}

std::string AuditWatcher::extract_event_id(const std::string& line) const {
    // vd: msg=audit(1623456789.123:456):
    std::size_t start_pos = line.find("msg=audit(");
    if (start_pos == std::string::npos) {
        return "";
    }
    start_pos += 10;
    std::size_t end_pos = line.find("):", start_pos);
    if (end_pos == std::string::npos) {
        return "";
    }
    return line.substr(start_pos, end_pos - start_pos);
}

std::string AuditWatcher::extract_field(const std::string& line, const std::string& key) const {
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

EventType AuditWatcher::determine_event_type(const std::string& syscall_num, const std::string& nametype) const {
    if (nametype == "CREATE") return EventType::CREATE;
    if (nametype == "DELETE") return EventType::DELETE;
    
    if (!syscall_num.empty()) {
        try {
            int syscall_id = std::stoi(syscall_num);
            // X86_64 syscalls
            // 87 = unlink, 263 = unlinkat
            if (syscall_id == 87 || syscall_id == 263) return EventType::DELETE;
            
            // 90 = chmod, 268 = fchmodat, 92 = chown, 260 = fchownat
            if (syscall_id == 90 || syscall_id == 268 || syscall_id == 92 || syscall_id == 260) return EventType::ATTRIB;

            // 2 = open, 257 = openat
            // 82 = rename, 264 = renameat
            return EventType::MODIFY;
        } catch (...) {}
    }
    return EventType::MODIFY;
}

void AuditWatcher::process_line(const std::string& line) {
    // Loc IP
    session_tracker.process_line(line);

    std::string event_id = extract_event_id(line);
    if (event_id.empty()) {
        return;
    }

    if (current_event_id != event_id) {
        if (!current_event_id.empty()) {
            flush_event(current_event_id);
        }
        current_event_id = event_id;
    }

    if (line.find("type=SYSCALL") != std::string::npos) {
        std::string key = extract_field(line, "key=");
        if (!audit_key.empty() && key != audit_key) {
            return;
        }

        std::string ses_str = extract_field(line, "ses=");
        std::string exe = extract_field(line, "exe=");
        std::string syscall = extract_field(line, "syscall=");
        std::string uid_str = extract_field(line, "uid=");
        std::string auid_str = extract_field(line, "auid=");
        std::string pid_str = extract_field(line, "pid=");
        
        if (!ses_str.empty()) {
            try {
                unsigned long ses_raw = std::stoul(ses_str);

                if (ses_raw == 4294967295UL) { // Bo qua internal session
                    pending_events[event_id].session_id = -1;
                } else {
                    pending_events[event_id].session_id = static_cast<int>(ses_raw);
                }
            } catch (...) {}
        }

        if (!uid_str.empty()) { 
            try { 
                pending_events[event_id].uid = std::stoul(uid_str); 
            } catch(...) {} 
        }

        if (!auid_str.empty()) { 
            try { 
                pending_events[event_id].auid = std::stoul(auid_str); 
            } catch(...) {} 
        }

        if (!pid_str.empty()) { 
            try { 
                pending_events[event_id].pid = std::stoi(pid_str); 
            } catch(...) {} 
        }
        
        // Extract timestamp
        std::size_t dot_pos = event_id.find('.');
        std::size_t colon_pos = event_id.find(':');
        if (dot_pos != std::string::npos) {
            try {
                uint64_t seconds = std::stoull(event_id.substr(0, dot_pos));
                uint64_t millis = 0;
                if (colon_pos != std::string::npos && colon_pos > dot_pos) {
                    millis = std::stoull(event_id.substr(dot_pos + 1, colon_pos - dot_pos - 1));
                }
                pending_events[event_id].timestamp = (seconds * 1000) + millis;
            } catch(...) {}
        }

        if (!exe.empty()) {
            pending_events[event_id].exe = exe;
        }
        if (!syscall.empty()) {
            pending_events[event_id].syscall_num = syscall;
        }
    } 
    else if (line.find("type=PATH") != std::string::npos) {
        std::string name = extract_field(line, "name=");
        std::string nametype = extract_field(line, "nametype=");

        if (!name.empty() && name != "(null)") {
            pending_events[event_id].files.push_back({name, nametype});
        }
    }
}

void AuditWatcher::flush_event(const std::string& event_id) {
    auto it = pending_events.find(event_id);
    if (it != pending_events.end()) {
        const PendingEvent& pe = it->second;
        
        // Neu co it nhat 1 file bi thay doi -> Ban event
        if (!pe.files.empty() && callback) {
            std::string source_ip = "127.0.0.1"; // Default cho Cron/Local
            if (pe.session_id != -1) {
                source_ip = session_tracker.get_source_ip(pe.session_id);
            }
            
            std::string username = "unknown";
            struct passwd pwd;
            struct passwd* result = nullptr;
            char buf[256];
            if (getpwuid_r(pe.uid, &pwd, buf, sizeof(buf), &result) == 0 && result) {
                username = result->pw_name;
            }
            
            for (const auto& file_record : pe.files) {
                AuditEvent ev;
                ev.audit_serial = event_id;
                ev.file_path = file_record.path;
                ev.file_name = file_record.path.substr(file_record.path.find_last_of('/') + 1);
                ev.process_name = pe.exe;
                ev.exe_path = pe.exe;
                ev.session_id = pe.session_id;
                ev.source_ip = source_ip;
                ev.uid = pe.uid;
                ev.auid = pe.auid;
                ev.pid = pe.pid;
                ev.username = username;
                ev.timestamp = pe.timestamp;
                
                ev.event_type = determine_event_type(pe.syscall_num, file_record.nametype);
                
                callback(ev);
            }
        }
        pending_events.erase(it);
    }
}

void AuditWatcher::tail_loop(std::string log_file_path) {
    std::ifstream file(log_file_path);
    
    if (file.is_open()) {
        // Dua con tro ve cuoi file, chi doc log sinh ra tu thoi diem khoi dong
        file.seekg(0, std::ios::end);
    }

    std::string line;
    while (running) {
        if (!file.is_open()) {
            file.open(log_file_path);
            if (file.is_open()) {
                file.seekg(0, std::ios::end);
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
        }

        if (std::getline(file, line)) {
            process_line(line);
        } else {
            file.clear();
            
            // Kiem tra log rotation (truncate)
            std::streampos current_pos = file.tellg();
            file.seekg(0, std::ios::end);
            std::streampos end_pos = file.tellg();
            
            if (end_pos < current_pos) {
                // File da bi truncate (logrotate)
                file.seekg(0, std::ios::beg);
            } else {
                // File van the, tra lai vi tri cu
                file.seekg(current_pos);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    if (!current_event_id.empty()) {
        flush_event(current_event_id);
        current_event_id.clear();
    }
}
