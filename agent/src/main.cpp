#include "config.h"
#include "logger.h"
#include "event.h"
#include <iostream>

int main() {
    // Buoc 1: Nap cau hinh tu file YAML
    std::string config_path = "config/agent.yaml";
    if (!Config::instance().load_config(config_path)) {
        std::cerr << "Không thể nạp cấu hình. Kiểm tra lại file: " << config_path << std::endl;
        return 1;
    }

    // Buoc 2: Khoi tao Logger
    Logger::instance().init(
        Config::instance().get_log_path(),
        LogLevel::INFO
    );

    LOG_INFO("VDT Agent khởi động");
    LOG_INFO("Agent ID: " + Config::instance().get_agent_id());
    LOG_INFO("Project ID: " + Config::instance().get_project_id());
    LOG_INFO("Central URL: " + Config::instance().get_central_url());
    LOG_INFO("Log Level: " + Config::instance().get_log_level());

    // In danh sach thu muc giam sat
    for (const auto& path : Config::instance().get_watch_paths()) {
        LOG_INFO("Watch Path: " + path);
    }

    // Buoc 3: Test module Event
    LOG_INFO("Test EventHelper");

    std::string hostname = EventHelper::get_hostname();
    std::string server_ip = EventHelper::get_server_ip();
    std::string time_now = EventHelper::get_iso8601_time();
    std::string uuid = EventHelper::generate_uuid();

    LOG_INFO("Hostname: " + hostname);
    LOG_INFO("Server IP: " + server_ip);
    LOG_INFO("Time ISO8601: " + time_now);
    LOG_INFO("UUID: " + uuid);

    // Buoc 4: Test dong goi MonitorEvent
    AuditEvent audit;
    audit.file_path = "/etc/nginx/nginx.conf";
    audit.file_name = "nginx.conf";
    audit.event_type = EventType::MODIFY;
    audit.username = "nguyenthang";
    audit.uid = 1000;
    audit.pid = 12345;
    audit.process_name = "vim";

    MonitorEvent monitor = EventHelper::to_monitor_event(
        audit,
        Config::instance().get_agent_id(),
        Config::instance().get_project_id(),
        hostname,
        server_ip
    );

    LOG_INFO("MonitorEvent da dong goi");
    LOG_INFO("Event ID: " + monitor.event_id);
    LOG_INFO("Agent ID: " + monitor.agent_id);
    LOG_INFO("File: " + monitor.audit_data.file_path);
    LOG_INFO("User: " + monitor.audit_data.username);
    LOG_INFO("Action: " + EventHelper::event_type_to_string(monitor.audit_data.event_type));

    LOG_INFO("VDT Agent kết thúc");
    return 0;
}
