import logging
from .ip_checker import IpChecker
from .jenkins_client import JenkinsClient
from .elasticsearch_client import ElasticsearchClient
from .remediation_client import RemediationClient

logger = logging.getLogger(__name__)

class ReconciliationEngine:
    def __init__(self, config: dict):
        self.config = config
        
        self.ip_checker = IpChecker(config.get("trusted_ips", []))
        
        jenkins_cfg = config.get("jenkins", {})
        self.enable_crosscheck = jenkins_cfg.get("enable_crosscheck", False)
        self.jenkins_client = JenkinsClient(
            base_url=jenkins_cfg.get("base_url", ""),
            username=jenkins_cfg.get("username", ""),
            api_token=jenkins_cfg.get("api_token", ""),
            job_name=jenkins_cfg.get("job_name", ""),
            timeout=jenkins_cfg.get("timeout_seconds", 5)
        )
        
        es_cfg = config.get("elasticsearch", {})
        self.es_client = ElasticsearchClient(
            es_url=es_cfg.get("url", ""),
            index_name=es_cfg.get("index", "")
        )
        
        self.remediation_client = RemediationClient(config.get("remediation", {}))
        
        self.total_events = 0
        self.total_violations = 0
        
        logger.info("Đã khởi tạo ReconciliationEngine thành công.")

    def process(self, event: dict) -> None:
        self.total_events += 1
        
        audit_data = event.get("audit_data", {})
        source_ip = audit_data.get("source_ip", "")
        file_path = audit_data.get("file_path", "")

        ip_trusted = self.ip_checker.is_trusted(source_ip)
        
        jenkins_building = False
        if self.enable_crosscheck:
            jenkins_building = self.jenkins_client.is_build_running()

        if ip_trusted and jenkins_building:
            logger.info(f"Hợp lệ: IP {source_ip} (Trusted) đang thao tác trên '{file_path}' khi Jenkins có job chạy.")
            return

        if ip_trusted and not jenkins_building:
            logger.warning(f"CẢNH BÁO: IP {source_ip} (Trusted) thao tác trên '{file_path}' nhưng KHÔNG CÓ job Jenkins nào đang chạy.")
            event["severity"] = "WARNING"
            self.es_client.send_violation(event)
            self.total_violations += 1
            return

        if not ip_trusted and jenkins_building:
            logger.warning(f"CẢNH BÁO: IP {source_ip} (Untrusted) thao tác trên '{file_path}' nhưng Jenkins CÓ job đang chạy.")
            event["severity"] = "WARNING"
            self.es_client.send_violation(event)
            self.total_violations += 1
            return

        logger.error(f"VI PHẠM NGHIÊM TRỌNG: IP {source_ip} (Untrusted) thao tác trái phép trên '{file_path}' khi Jenkins KHÔNG chạy!")
        event["severity"] = "HIGH"
        self.es_client.send_violation(event)
        self.total_violations += 1
        
        self.remediation_client.trigger(event)

    def get_stats(self) -> dict:
        return {
            "total_events": self.total_events,
            "total_violations": self.total_violations
        }
