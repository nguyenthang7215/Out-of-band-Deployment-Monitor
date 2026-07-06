from fastapi import APIRouter, Request, BackgroundTasks
from fastapi.responses import JSONResponse
from pydantic import ValidationError
import logging
from schemas.event_schema import MonitorEvent

logger = logging.getLogger(__name__)

router = APIRouter()

@router.post("/events")
async def receive_event(request: Request, background_tasks: BackgroundTasks):
    engine = request.app.state.engine
    try:
        raw_data = await request.json()
        audit_data = raw_data.get("audit_data", {})
        logger.info(f"Nhận được event mới: {raw_data.get('event_id', 'unknown')} từ IP: {audit_data.get('source_ip', 'unknown')}")
        
        validated_event = MonitorEvent(**raw_data)
        
        if not engine:
            return JSONResponse(status_code=503, content={"status": "error", "message": "Service not ready"})
        
        # Fire-and-forget: Day vao queue ngam de xu li
        background_tasks.add_task(engine.process, validated_event.model_dump())
        
        return {"status": "success", "message": "Event queued for processing"}
        
    except ValidationError as e:
        logger.error(f"Data nhận được không đúng định dạng (thiếu fields): {e}")
        return JSONResponse(status_code=400, content={"status": "error", "message": "Missing required fields"})
    except Exception as e:
        logger.error(f"Lỗi xử lý sự kiện: {e}")
        return JSONResponse(status_code=500, content={"status": "error", "message": str(e)})

@router.get("/stats")
def get_stats(request: Request):
    engine = request.app.state.engine
    if engine:
        return engine.get_stats()
    return {"total_events": 0, "total_violations": 0}

@router.post("/alerts")
async def receive_grafana_alert(request: Request):
    try:
        alert_data = await request.json()
        alert_title = alert_data.get('title', 'Unknown Alert')
        
        logger.warning(f"[GRAFANA ALERT] Tiếp nhận cảnh báo từ Grafana | Tiêu đề: {alert_title}")
        
        return {"status": "success", "message": "Đã ghi nhận cảnh báo"}
        
    except Exception as e:
        logger.error(f"Lỗi xử lý Grafana Webhook: {str(e)}")
        return JSONResponse(
            status_code=500, 
            content={"status": "error", "message": "Lỗi máy chủ nội bộ"}
        )
