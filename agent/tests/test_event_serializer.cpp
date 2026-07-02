#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "event_serializer.h"
#include "event.h"

// Test 1: Chuyển MonitorEvent sang JSON string
TEST(EventSerializerTest, SerializeMonitorEvent) {
    // Tạo giả lập dữ liệu
    AuditEvent audit;
    audit.audit_serial = "12345:678";
    audit.file_path = "/etc/passwd";
    audit.file_name = "passwd";
    audit.event_type = EventType::MODIFY;
    audit.username = "root";
    audit.source_ip = "192.168.1.5";
    audit.timestamp_iso = "2026-07-02T10:00:00Z";

    MonitorEvent event;
    event.event_id = "test-uuid-123";
    event.agent_id = "agent-01";
    event.server_hostname = "prod-01";
    event.server_ip = "10.0.0.1";
    event.project_id = "vdt-project";
    event.sensor = "auditd";
    event.audit_data = audit;

    // Chạy hàm cần test
    std::string json_str = EventSerializer::to_json(event);

    // Parse lại JSON từ string để kiểm tra tính hợp lệ
    nlohmann::json j = nlohmann::json::parse(json_str);

    // Kiểm tra cấp ngoài cùng (Root level)
    EXPECT_EQ(j["event_id"], "test-uuid-123");
    EXPECT_EQ(j["agent_id"], "agent-01");
    EXPECT_EQ(j["server_hostname"], "prod-01");
    EXPECT_EQ(j["sensor"], "auditd");

    // Kiểm tra cấp bên trong (Nested audit_data)
    ASSERT_TRUE(j.contains("audit_data"));
    auto& j_audit = j["audit_data"];
    
    EXPECT_EQ(j_audit["audit_serial"], "12345:678");
    EXPECT_EQ(j_audit["file_path"], "/etc/passwd");
    EXPECT_EQ(j_audit["username"], "root");
    EXPECT_EQ(j_audit["source_ip"], "192.168.1.5");
    
    // Kiểm tra Enum đã được chuyển thành chuỗi Text chuẩn chưa
    EXPECT_EQ(j_audit["event_type"], "MODIFY");
}
