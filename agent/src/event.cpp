#include "event.h"
#include <iomanip>
#include <chrono>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return "127.0.0.1";
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("8.8.8.8");
    serv.sin_port = htons(53);

    if (connect(sock, (const struct sockaddr*)&serv, sizeof(serv)) < 0) {
        close(sock);
        return "127.0.0.1";
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    getsockname(sock, (struct sockaddr*)&name, &namelen);

    std::string ip = inet_ntoa(name.sin_addr);
    close(sock);
    return ip;
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