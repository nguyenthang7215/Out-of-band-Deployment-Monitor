from pydantic import BaseModel

class AuditData(BaseModel):
    file_path: str
    file_name: str
    event_type: str
    username: str
    uid: int
    auid: int
    pid: int
    process_name: str
    exe_path: str
    session_id: int
    source_ip: str
    timestamp: int
    timestamp_iso: str
    audit_serial: str

class MonitorEvent(BaseModel):
    event_id: str
    agent_id: str
    server_hostname: str
    server_ip: str
    project_id: str
    sensor: str
    audit_data: AuditData
