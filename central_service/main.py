import yaml
import logging
from fastapi import FastAPI
from contextlib import asynccontextmanager

from services.reconciliation_engine import ReconciliationEngine
from api.events import router as events_router

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s")
logger = logging.getLogger("main")

@asynccontextmanager
async def lifespan(app: FastAPI):
    try:
        with open("config.yaml", "r") as f:
            config = yaml.safe_load(f)
            
        log_level = config.get("server", {}).get("log_level", "INFO").upper()
        logging.getLogger().setLevel(log_level)
        
        app.state.engine = ReconciliationEngine(config)
        logger.info("Service khởi động thành công.")
    except Exception as e:
        logger.error(f"Lỗi khởi động service: {e}")
        raise e
        
    yield
    logger.info("Service đang tắt...")

# Khoi tao server FastAPI
app = FastAPI(title="Out-of-band Deployment Monitor - Central Service", lifespan=lifespan)

app.include_router(events_router, prefix="/api/v1", tags=["Events"])

@app.get("/health", tags=["System"])
def health_check():
    return {"status": "ok"}
