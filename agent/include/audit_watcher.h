#pragma once

#include "event.h"
#include "session_tracker.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>

class AuditWatcher {
public:
    using AuditEventCallback = std::function<void(const AuditEvent&)>; // alias

    AuditWatcher(SessionTracker& tracker);
    ~AuditWatcher();

    void set_callback(AuditEventCallback cb);
    void start(const std::string& log_file_path = "/var/log/audit/audit.log");
    void stop();

private:
    struct FileRecord {
        std::string path;
        std::string nametype;
    };

    struct PendingEvent {
        int session_id = -1;
        std::string exe;
        std::string syscall_num;
        uint32_t uid = 0;
        uint32_t auid = 0;
        int pid = -1;
        uint64_t timestamp = 0;
        std::string timestamp_iso;
        std::vector<FileRecord> files;
    };

    void tail_loop(std::string log_file_path);
    void process_line(const std::string& line);
    void flush_event(const std::string& event_id);
    
    EventType determine_event_type(const std::string& syscall_num, const std::string& nametype) const;
    std::string extract_event_id(const std::string& line) const;
    std::string extract_field(const std::string& line, const std::string& key) const;

    SessionTracker& session_tracker;
    AuditEventCallback callback;

    std::unordered_map<std::string, PendingEvent> pending_events;
    std::string current_event_id;

    std::thread watcher_thread;
    std::atomic<bool> running{false};
};
