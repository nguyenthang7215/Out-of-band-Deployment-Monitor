import logging
from datetime import datetime, timezone
import requests

logger = logging.getLogger(__name__)

class ElasticsearchClient:
    def __init__(self, es_url: str, index_name: str):
        self.es_url = es_url
        self.index_name = index_name
        self.is_ready = False
        
        try:
            resp = requests.get(self.es_url, timeout=3)
            if resp.status_code == 200:
                logger.info(f"Kết nối Elasticsearch thành công: {self.es_url}")
                self.is_ready = True
            else:
                logger.warning(f"Elasticsearch phản hồi mã {resp.status_code} — sẽ retry khi gửi data")
        except Exception as e:
            logger.error(f"Lỗi khởi tạo Elasticsearch (chưa bật?): {e}")

    def send_violation(self, event: dict) -> bool:
        # Them cac field quan trong vao event
        violation_doc = event.copy()
        violation_doc["violation_time"] = datetime.now(timezone.utc).isoformat()
        violation_doc["severity"] = event.get("severity", "HIGH")
        
        # Day cac fields quan trong tu audit_data ra root level de Grafana doc duoc
        audit_data = event.get("audit_data", {})
        violation_doc["timestamp_iso"] = audit_data.get("timestamp_iso", violation_doc["violation_time"])
        violation_doc["file_path"] = audit_data.get("file_path", "")
        violation_doc["event_type"] = audit_data.get("event_type", "")
        violation_doc["username"] = audit_data.get("username", "")
        violation_doc["source_ip"] = audit_data.get("source_ip", "")

        try:
            url = f"{self.es_url}/{self.index_name}/_doc"
            resp = requests.post(url, json=violation_doc, headers={"Content-Type": "application/json"}, timeout=5)
            if resp.status_code in (200, 201):
                logger.info(f"Đã ghi vi phạm vào ES thành công (index: {self.index_name})")
                return True
            else:
                logger.error(f"Lỗi khi ghi violation vào Elasticsearch: {resp.text}")
                return False
        except Exception as e:
            logger.error(f"Lỗi khi ghi violation vào Elasticsearch qua requests: {e}")
            return False
