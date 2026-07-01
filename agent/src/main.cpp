#include "config.h"
#include "logger.h"
#include "event.h"
#include "event_queue.h"
#include "network_client.h"
#include "session_tracker.h"
#include "audit_watcher.h"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> running{true};

void signal_handler(int signum) {
    LOG_INFO("Nhận tín hiệu dừng (Signal: " + std::to_string(signum) + "). Đang dọn dẹp và tắt Agent...");
    running = false;
}

void setup_audit_rule() {
    auto paths = Config::instance().get_watch_paths();
    for (const auto& path : paths) {
        std::string cmd = "auditctl -w " + path + " -p wa";
        LOG_INFO("Thêm rule audit: " + cmd);
        int ret = system(cmd.c_str());
        if (ret != 0) {
            LOG_INFO("Cảnh báo: Không thể thêm rule audit cho " + path + " (Yêu cầu quyền root)");
        }
    }
}

int main() {
    // Dang ky tin hieu he thong de graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::string config_path = "/etc/vdt-agent/agent.yaml";
    if (!Config::instance().load_config(config_path)) {
        config_path = "config/agent.yaml";
        if (!Config::instance().load_config(config_path)) {
            std::cerr << "Lỗi: Không thể nạp cấu hình. Vui lòng kiểm tra file agent.yaml" << std::endl;
            return 1;
        }
    }

    Logger::instance().init(
        Config::instance().get_log_path(),
        LogLevel::INFO
    );

    LOG_INFO("VDT Out-of-band Monitor Agent bắt đầu khởi động");
    LOG_INFO("Đường dẫn cấu hình: " + config_path);
    LOG_INFO("Mã Agent: " + Config::instance().get_agent_id());
    LOG_INFO("Mã Dự án: " + Config::instance().get_project_id());


    std::string hostname = EventHelper::get_hostname();
    std::string server_ip = EventHelper::get_server_ip();

    EventQueue event_queue;
    NetworkClient network_client(Config::instance().get_central_url(), event_queue);
    
    network_client.start();

    SessionTracker session_tracker;
    AuditWatcher audit_watcher(session_tracker);
    
    setup_audit_rule();

    audit_watcher.set_callback([&](const AuditEvent& audit_event) {
        LOG_INFO("Phát hiện thay đổi tại file: " + audit_event.file_path + " (Tiến trình: " + audit_event.process_name + ")");
        
        MonitorEvent monitor_event = EventHelper::to_monitor_event(
            audit_event,
            Config::instance().get_agent_id(),
            Config::instance().get_project_id(),
            hostname,
            server_ip
        );

        event_queue.push(monitor_event);
    });

    audit_watcher.start("/var/log/audit/audit.log");

    LOG_INFO("Agent đang hoạt động bình thường (Nhấn Ctrl+C để dừng)");

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOG_INFO("Đang dọn dẹp các luồng chạy ngầm...");
    audit_watcher.stop();
    network_client.stop();
    
    LOG_INFO("VDT Agent kết thúc an toàn.");
    return 0;
}
