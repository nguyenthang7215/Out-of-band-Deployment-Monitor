import logging
from datetime import datetime, timezone
from elasticsearch import Elasticsearch

logger = logging.getLogger(__name__)

class ElasticsearchClient:
    def __init__(self, es_url: str, index_name: str):
        self.es_url = es_url
        self.index_name = index_name
        
        try:
            self.client = Elasticsearch([self.es_url])

            if self.client.ping():
                logger.info(f"Kết nối Elasticsearch thành công: {self.es_url}")
            else:
                logger.warning("Elasticsearch không phản hồi ping — sẽ retry khi gửi data")
        except Exception as e:
            logger.error(f"Lỗi khởi tạo Elasticsearch client: {e}")
            self.client = None

    def send_violation(self, event: dict) -> bool:
        if not self.client:
            logger.error("Elasticsearch client chưa sẵn sàng, không thể gửi violation")
            return False

        # Them cac field quan trong vao event
        violation_doc = event.copy()
        violation_doc["violation_time"] = datetime.now(timezone.utc).isoformat()
        violation_doc["severity"] = "HIGH"

        try:
            # Ghi vao ES. Neu index chua ton tai, ES se tu dong tao (theo cau hinh mac dinh)
            resp = self.client.index(index=self.index_name, document=violation_doc)
            logger.info(f"Đã ghi vi phạm vào ES thành công (index: {self.index_name}, id: {resp.get('_id')})")
            return True
        except Exception as e:
            logger.error(f"Lỗi khi ghi violation vào Elasticsearch: {e}")
            return False
