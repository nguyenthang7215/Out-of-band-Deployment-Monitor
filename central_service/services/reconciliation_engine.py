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

        is_trusted_ip = self.ip_checker.is_trusted(source_ip)
        is_building = False
        
        if self.enable_crosscheck:
            is_building = self.jenkins_client.is_build_running()
        else:
            is_building = True

        if is_building:
            logger.info(f"Hợp lệ: Jenkins đang chạy deploy. Thao tác trên {file_path} (từ IP {source_ip}) được cho phép.")
            return

        # Neu khong building thi deu la vi pham
        if is_trusted_ip:
            logger.warning(f"VI PHẠM: IP {source_ip} (Trusted IP) cấu hình thủ công trái phép khi Jenkins không chạy deploy!")
        else:
            logger.error(f"VI PHẠM NGHIÊM TRỌNG: IP {source_ip} KHÔNG trusted đang cấu hình thủ công trái phép khi Jenkins không chạy deploy!")
        
        event["severity"] = "HIGH"
        
        self.es_client.send_violation(event)
        self.total_violations += 1
        
        # Trigger tu dong sua loi
        if event.get("severity") == "HIGH":
            self.remediation_client.trigger(event)

    def get_stats(self) -> dict:
        return {
            "total_events": self.total_events,
            "total_violations": self.total_violations
        }
