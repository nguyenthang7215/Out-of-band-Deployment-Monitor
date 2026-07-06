#include "event_serializer.h"
#include <nlohmann/json.hpp>

std::string EventSerializer::to_json(const MonitorEvent& event) {
    nlohmann::json json_data;
    
    json_data["event_id"] = event.event_id;
    json_data["agent_id"] = event.agent_id;
    json_data["server_hostname"] = event.server_hostname;
    json_data["server_ip"] = event.server_ip;
    json_data["project_id"] = event.project_id;
    json_data["sensor"] = event.sensor;

    json_data["audit_data"]["file_path"] = event.audit_data.file_path;
    json_data["audit_data"]["file_name"] = event.audit_data.file_name;
    json_data["audit_data"]["event_type"] = EventHelper::event_type_to_string(event.audit_data.event_type);
    json_data["audit_data"]["username"] = event.audit_data.username;
    json_data["audit_data"]["uid"] = event.audit_data.uid;
    json_data["audit_data"]["auid"] = event.audit_data.auid;
    json_data["audit_data"]["pid"] = event.audit_data.pid;
    json_data["audit_data"]["process_name"] = event.audit_data.process_name;
    json_data["audit_data"]["exe_path"] = event.audit_data.exe_path;
    json_data["audit_data"]["session_id"] = event.audit_data.session_id;
    json_data["audit_data"]["source_ip"] = event.audit_data.source_ip;
    json_data["audit_data"]["timestamp"] = event.audit_data.timestamp;
    json_data["audit_data"]["timestamp_iso"] = event.audit_data.timestamp_iso;
    json_data["audit_data"]["audit_serial"] = event.audit_data.audit_serial;

    return json_data.dump();
}