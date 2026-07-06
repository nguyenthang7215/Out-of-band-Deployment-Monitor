import requests
import logging

logger = logging.getLogger(__name__)

class JenkinsClient:
    def __init__(self, base_url: str, username: str, api_token: str, job_name: str, timeout: int):
        self.base_url = base_url.rstrip("/")
        self.username = username
        self.api_token = api_token
        self.job_name = job_name
        self.timeout = timeout
        logger.info(f"Đã khởi tạo JenkinsClient để giám sát job: {self.job_name}")

    def is_build_running(self) -> bool:
        url = f"{self.base_url}/job/{self.job_name}/lastBuild/api/json?tree=building"
        try:
            response = requests.get(url, auth=(self.username, self.api_token), timeout=self.timeout)
            response.raise_for_status() # Ban exception khi HTTP status code >= 400
            
            data = response.json()
            is_building = data.get("building", False)
            
            logger.info(f"Trạng thái Jenkins job '{self.job_name}': building={is_building}")
            return is_building

        except requests.exceptions.HTTPError as e:
            if e.response.status_code == 404:
                logger.warning(f"Jenkins job '{self.job_name}' chưa có build nào (404)")
            else:
                logger.warning(f"Jenkins API lỗi HTTP {e.response.status_code}: {e}")
            return False
        except requests.exceptions.RequestException as e:
            logger.warning(f"Lỗi kết nối Jenkins: {e}")
            return False
        except Exception as e:
            logger.error(f"Lỗi không xác định trong JenkinsClient: {e}")
            return False
