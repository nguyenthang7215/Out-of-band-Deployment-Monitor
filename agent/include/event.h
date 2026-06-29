#pragma once

#include <string>
#include <cstdint>

enum class EventType {
    CREATE = 0,
    MODIFY = 1,
    DELETE = 2,
    ATTRIB = 3,
    UNKNOWN = 99
};

// Du lieu tho tu audit log
struct AuditEvent {
    std::string file_path;
    std::string file_name;
    EventType event_type = EventType::UNKNOWN;
    std::string username;
    uint32_t uid = 0;
    uint32_t auid = 0;
    int pid = -1;
    std::string process_name;
    std::string exe_path;
    int session_id = -1;
    std::string source_ip;
    uint64_t timestamp = 0;
    std::string timestamp_iso;
    std::string audit_serial;
};

// Du lieu gui len service 
struct MonitorEvent {
    std::string event_id;
    std::string agent_id;
    std::string server_hostname;
    std::string server_ip;
    std::string project_id;
    std::string sensor;
    AuditEvent audit_data;
};

class EventHelper { 
public: 
    static MonitorEvent to_monitor_event(
        const AuditEvent& audit_event,
        const std::string& agent_id,
        const std::string& project_id,
        const std::string& server_hostname,
        const std::string& server_ip
    );

    static std::string generate_uuid();
    static std::string get_hostname();
    static std::string get_server_ip();
    static std::string get_iso8601_time();
    static std::string event_type_to_string(EventType type);
};