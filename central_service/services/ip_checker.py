import logging

logger = logging.getLogger(__name__)

class IpChecker:
    def __init__(self, trusted_ips: list[str]):
        self.trusted_ips = set(trusted_ips)
        logger.info(f"Đã khởi tạo IpChecker với {len(self.trusted_ips)} IP hợp lệ.")

    def is_trusted(self, source_ip: str) -> bool:
        if not source_ip or source_ip == "unknown":
            logger.warning("source_ip trống hoặc unknown — coi là không trusted")
            return False
            
        is_safe = source_ip in self.trusted_ips
        if is_safe:
            logger.debug(f"IP {source_ip} trusted")
        else:
            logger.warning(f"IP {source_ip} KHÔNG trusted")
        return is_safe
