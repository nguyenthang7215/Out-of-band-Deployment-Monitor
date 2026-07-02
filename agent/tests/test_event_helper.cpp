#include <gtest/gtest.h>
#include "event.h"
#include <regex>

// Test generate_uuid
TEST(EventHelperTest, GenerateUUID) {
    std::string uuid1 = EventHelper::generate_uuid();
    std::string uuid2 = EventHelper::generate_uuid();

    // Khong rong
    EXPECT_FALSE(uuid1.empty());
    
    // Khong trung lap
    EXPECT_NE(uuid1, uuid2);

    // Kiem tra format co ban cua UUID (co dau gach ngang)
    // UUID v4 format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    std::regex uuid_regex("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$");
    EXPECT_TRUE(std::regex_match(uuid1, uuid_regex));
}

// Test get_hostname
TEST(EventHelperTest, GetHostname) {
    std::string hostname = EventHelper::get_hostname();
    EXPECT_FALSE(hostname.empty());
    EXPECT_NE(hostname, "unknown_host");
}

// Test event_type_to_string
TEST(EventHelperTest, EventTypeToString) {
    EXPECT_EQ(EventHelper::event_type_to_string(EventType::CREATE), "CREATE");
    EXPECT_EQ(EventHelper::event_type_to_string(EventType::MODIFY), "MODIFY");
    EXPECT_EQ(EventHelper::event_type_to_string(EventType::DELETE), "DELETE");
    EXPECT_EQ(EventHelper::event_type_to_string(EventType::ATTRIB), "ATTRIB");
    EXPECT_EQ(EventHelper::event_type_to_string(EventType::UNKNOWN), "UNKNOWN");
}

// Test to_monitor_event
TEST(EventHelperTest, ToMonitorEvent) {
    AuditEvent audit;
    audit.audit_serial = "123:456";
    audit.file_path = "/tmp/test.txt";
    audit.file_name = "test.txt";
    audit.event_type = EventType::CREATE;
    audit.username = "root";
    audit.source_ip = "10.0.0.1";
    audit.timestamp_iso = "2023-01-01T12:00:00Z";

    MonitorEvent monitor = EventHelper::to_monitor_event(
        audit, "agent-01", "project-x", "server-host", "192.168.1.1"
    );

    // Kiem tra UUID duoc tu dong tao
    EXPECT_FALSE(monitor.event_id.empty());
    
    // Kiem tra cac truong them vao
    EXPECT_EQ(monitor.agent_id, "agent-01");
    EXPECT_EQ(monitor.project_id, "project-x");
    EXPECT_EQ(monitor.server_hostname, "server-host");
    EXPECT_EQ(monitor.server_ip, "192.168.1.1");
    EXPECT_EQ(monitor.sensor, "auditd");
    
    // Kiem tra audit_data duoc copy dung
    EXPECT_EQ(monitor.audit_data.audit_serial, "123:456");
    EXPECT_EQ(monitor.audit_data.file_path, "/tmp/test.txt");
    EXPECT_EQ(monitor.audit_data.event_type, EventType::CREATE);
}
