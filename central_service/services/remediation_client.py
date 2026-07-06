import logging
import requests

logger = logging.getLogger(__name__)

class RemediationClient:
    def __init__(self, config: dict):
        self.enabled = config.get("enabled", False)
        self.ansible_url = config.get("ansible_url", "").rstrip("/")
        self.ansible_token = config.get("ansible_token", "")
        self.playbook = config.get("playbook", "")
        self.fail_count = 0
        self.MAX_FAILURES = 3
        
        logger.info(f"Đã khởi tạo RemediationClient. Tình trạng hoạt động (enabled): {self.enabled}")

    def trigger(self, event: dict) -> bool:
        if not self.enabled:
            logger.info("Remediation đang TẮT. Chỉ log sự kiện mà không gọi Ansible")
            return False
        
        # Circuit Breaker
        if self.fail_count >= self.MAX_FAILURES:
            logger.error("Circuit Breaker kích hoạt: RemediationClient đã thất bại quá nhiều lần liên tiếp, tạm thời ngắt")
            return False

        url = f"{self.ansible_url}/api/v2/job_templates/{self.playbook}/launch/"
        payload = {
            "extra_vars": {
                "violation_event": event
            }
        }

        try:
            logger.info(f"Đang kích hoạt quy trình khắc phục sự cố tại {url}")
            headers = {
                "Authorization": f"Bearer {self.ansible_token}",
                "Content-Type": "application/json"
            }
            response = requests.post(url, json=payload, headers=headers, timeout=10)
            response.raise_for_status()
            
            logger.info("Đã kích hoạt Remediation thành công.")
            self.fail_count = 0
            return True

        except requests.exceptions.RequestException as e:
            self.fail_count += 1
            logger.error(f"Lỗi khi kích hoạt Remediation (lần {self.fail_count}): {e}")
            return False
        except Exception as e:
            self.fail_count += 1
            logger.error(f"Lỗi không xác định khi kích hoạt Remediation (lần {self.fail_count}): {e}")
            return False
