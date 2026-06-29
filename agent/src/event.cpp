#include "event.h"
#include <iomanip>
#include <chrono>
#include <sstream>
#include <fstream>
#include <unistd.h>

MonitorEvent EventHelper::to_monitor_event(
    const AuditEvent& audit_event,
    const std::string& agent_id,
    const std::string& project_id,
    const std::string& server_hostname,
    const std::string& server_ip
) {
    MonitorEvent me;
    me.event_id = generate_uuid();
    me.agent_id = agent_id;
    me.project_id = project_id;
    me.server_hostname = server_hostname;
    me.server_ip = server_ip;
    me.sensor = "auditd";
    me.audit_data = audit_event;
    return me;
}

std::string EventHelper::generate_uuid() {
    std::ifstream file("/proc/sys/kernel/random/uuid"); // Doc tu file co san chua uuid random
    std::string uuid;
    if(file.is_open()) {
        std::getline(file, uuid);
        file.close();
    }
    else {
        uuid = "00000000-0000-0000-0000-000000000000";
    }
    return uuid;
}

std::string EventHelper::get_hostname() {
    char hostname[256];
    if(gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "unknown-host";
}

std::string EventHelper::get_server_ip() {
    // Tam thoi tra ve localhost
    // Se cap nhat IP that o giai doan network
    return "127.0.0.1";
}

std::string EventHelper::get_iso8601_time() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream formatted_time;
    formatted_time << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ"); // Theo iso 8601
    return formatted_time.str();
}

std::string EventHelper::event_type_to_string(EventType type) {
    switch(type) {
        case EventType::CREATE: return "CREATE";
        case EventType::MODIFY: return "MODIFY";
        case EventType::DELETE: return "DELETE";
        case EventType::ATTRIB: return "ATTRIB";
        default: return "UNKNOWN";
    }
}