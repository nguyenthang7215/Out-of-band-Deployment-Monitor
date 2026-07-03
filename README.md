# Out-of-band Deployment Monitor

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Python](https://img.shields.io/badge/Python-3.11+-yellow.svg)](https://www.python.org/)
[![Docker](https://img.shields.io/badge/Docker-Compose-2496ED.svg)](https://www.docker.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

> **Hệ thống Đối soát và Giám sát Triển khai Ngoài luồng (Reconciliation System)**
> Giải pháp DevSecOps giám sát tính toàn vẹn của Server Production, phát hiện *Shadow Deployment*, và tự động phục hồi cấu hình (Auto-Remediation).

---

## Mục lục
- [Bối cảnh & Bài toán](#bối-cảnh--bài-toán)
- [Mục tiêu hệ thống](#mục-tiêu-hệ-thống)
- [Kiến trúc hệ thống](#kiến-trúc-hệ-thống)
- [Luồng hoạt động (Workflow)](#luồng-hoạt-động-workflow)
- [Tính năng nổi bật](#tính-năng-nổi-bật)
- [Yêu cầu môi trường](#yêu-cầu-môi-trường)
- [Hướng dẫn triển khai](#hướng-dẫn-triển-khai)
- [Cấu trúc Project](#cấu-trúc-project)
- [Tiêu chí Đánh giá (E2E)](#tiêu-chí-đánh-giá-e2e)

---

## Bối cảnh & Bài toán
Hệ thống CI/CD hiện tại (sử dụng Jenkins, Ansible) đã thiết lập được quy trình kiểm soát chất lượng và triển khai tự động. Tuy nhiên, trong thực tế vận hành luôn tồn tại rủi ro **"Shadow Deployment" (triển khai ngầm)** — khi một cá nhân có quyền truy cập hệ thống tự ý SSH vào server production để sửa mã nguồn, thay đổi cấu hình (hotfix), hoặc upload artifact trực tiếp mà bỏ qua các bước quét bảo mật (Fortify, SonarQube).

Dự án này tập trung xây dựng một hệ thống đối soát (Reconciliation System) hoạt động độc lập với luồng CI/CD, liên tục giám sát tính toàn vẹn của mã nguồn và tự động phát hiện, cảnh báo cũng như khắc phục các can thiệp trái phép.

---

## Mục tiêu hệ thống
1. **Real-time Monitoring:** Phát hiện theo thời gian thực mọi thay đổi file/artifact tại thư mục chạy dự án trên production server.
2. **Giám sát Agent-based:** Agent viết bằng C++ siêu nhẹ. Thay vì liên tục quét thư mục (polling), Agent sử dụng `auditd` làm nguồn sự kiện kernel-level, đọc luồng log `/var/log/audit/audit.log` liên tục (cơ chế Log Tailing) để phân tích, ghép các record (SYSCALL + PATH theo audit serial) và đóng gói event. Nhờ vậy, mọi thay đổi trên đường dẫn cấu hình nhạy cảm đều được phát hiện tức thời.
3. **Drift Detection & Remediation:** Đảm bảo tính nhất quán giữa cấu hình chuẩn và thực tế, tự động khôi phục (Immutable Infrastructure) khi có vi phạm.

---

## Kiến trúc hệ thống

Dự án được chia làm 2 thành phần lõi chính và 3 thành phần phụ trợ (Infrastructure):

```text
┌─────────────────────────────────┐          ┌──────────────────────────────────┐
│  AGENT (C++ / Auditd)           │          │  CENTRAL SERVICE (Python)        │
│  - Chạy ngầm trên Server        │ ───────► │  - Xác minh logic (Cross-check)  │
│  - Bắt event mức Kernel         │   JSON   │  - Phân loại HIGH / WARNING      │
│  - Tiêu thụ CPU/RAM siêu nhỏ    │          │  - Đẩy Log & Cảnh báo            │
└─────────────────────────────────┘          └──────────────────────────────────┘
                                                               │
     ┌────────────────────────┐                                │
     │  JENKINS               │ ◄────── (API Check Job) ───────┤
     │  (Xác minh trạng thái) │                                │
     └────────────────────────┘                                │
     ┌────────────────────────┐                                │
     │  ANSIBLE AWX / TOWER   │ ◄────── (Trigger Playbook) ────┤
     │  (Auto-Remediation)    │                                │
     └────────────────────────┘                                │
     ┌────────────────────────┐                                │
     │  ELASTICSEARCH &       │ ◄────── (Lưu Audit Log) ───────┘
     │  GRAFANA (Dashboard)   │
     └────────────────────────┘
```

---

## Luồng hoạt động (Workflow)

1. **Thu thập (Collection):** `auditd` ở tầng OS Kernel ghi nhận sự kiện File System (Modify, Create, Delete, Attribute Change) và ghi ra log.
2. **Tiền xử lý (Pre-processing):** C++ Agent liên tục đọc và theo dõi file log của `auditd` (Log Tailing), trích xuất thông tin (File path, User ID, Timestamp) siêu tốc, đóng gói thành JSON và POST HTTP về Central Service.
3. **Đối soát (Reconciliation):** Central Service nhận JSON.
   - Kiểm tra IP nguồn xem có thuộc dải Trusted IP (Ansible Node) không.
   - Nếu IP trusted + Jenkins building=true: Hợp lệ → Drop event.
   - Nếu IP trusted + Jenkins building=false: WARNING → ghi Elasticsearch để audit.
   - Nếu IP untrusted + Jenkins building=true: WARNING → ghi Elasticsearch để audit.
   - Nếu IP untrusted + Jenkins building=false: HIGH → ghi Elasticsearch và trigger Ansible AWX.
4. **Hậu xử lý (Post-processing):**
   - Lưu trữ sự kiện vào **Elasticsearch**.
   - Nếu Severity = HIGH, gọi API sang **Ansible AWX** để chạy Playbook khôi phục (Auto-Remediation).
   - Hiển thị cảnh báo thời gian thực trên **Grafana Dashboard**.

---

## Tính năng nổi bật

### 1. Mức Bắt buộc (Core Features)
- **Giám sát File System cực sâu:** Sử dụng `auditd` làm nguồn cung cấp sự kiện từ lõi OS kết hợp cơ chế đọc log liên tục (Log Tailing), khắc phục hoàn toàn nhược điểm của các kỹ thuật quét thư mục (polling) truyền thống tốn kém tài nguyên.
- **Xác minh chéo thông minh:** Tự động gọi Remote Access API của Jenkins để biết chính xác có phiên Deploy hợp lệ nào đang diễn ra hay không.
- **Audit Log tập trung:** Toàn bộ lịch sử vi phạm được lưu trữ tại Elasticsearch `oob-monitor-violations` với đầy đủ metadata (User thực hiện, thời gian, file bị sửa).

### 2. Mức Khuyến khích (Advanced Features)
- **Tối ưu hiệu năng:** Agent C++ hoạt động dưới dạng daemon, ứng dụng kiến trúc Event-driven để đọc và xử lý log liên tục từ `auditd`. Cách tiếp cận này giúp **tối thiểu hóa** lượng RAM tiêu thụ và **giữ CPU ở mức rất thấp** trong điều kiện tải bình thường.
- **Cảnh báo trực quan (Observability):** Grafana Dashboard chuyên dụng (Auto-provisioned) hiển thị chỉ số "Configuration Drift", theo dõi số lượng vi phạm và tự động phân loại mức độ.
- **Auto-Remediation (Tự chữa lành):** Tích hợp Circuit Breaker thông minh. Khi phát hiện file cấu hình bị sửa trái phép, tự động gọi API Ansible để đè lại (overwrite) cấu hình gốc theo triết lý Immutable Infrastructure.

---

## Yêu cầu môi trường
- Linux OS (Ubuntu/CentOS) có cài đặt `auditd`
- C++ Compiler (g++ 9+)
- Python 3.11+
- Docker & Docker Compose (cho Infrastructure)

---

## Hướng dẫn Triển khai (Deployment Guide)

### Yêu cầu hệ thống (Prerequisites)
- **Máy chủ Central/Monitoring:** Đã cài đặt Docker và Docker Compose.
- **Máy chủ Target (Cần giám sát):** OS Linux (Ubuntu/CentOS), đã cài đặt `auditd`, `cmake`, `g++`, và `libcurl4-openssl-dev`.

---

### Bước 1: Khởi động Infrastructure (Môi trường Monitoring)
Toàn bộ Elasticsearch, Grafana, Jenkins và Central Service đã được cấu hình tự động hóa trong `docker-compose.yml`.

```bash
# Di chuyển vào thư mục deploy
cd deploy

# Khởi động toàn bộ stack ở chế độ ngầm (Detached mode)
docker-compose up -d

# Kiểm tra trạng thái các container
docker-compose ps
```

**Các dịch vụ sẽ khả dụng tại:**
- **Grafana:** `http://localhost:3000` (User/Pass: `admin`/`admin`. Dashboard *VDT Monitor* đã được tự động nạp thông qua Provisioning).
- **Jenkins:** `http://localhost:8081` (Lần đầu khởi động, lấy mật khẩu bằng lệnh: `docker exec vdt_jenkins cat /var/jenkins_home/secrets/initialAdminPassword`).
- **Central Service API:** `http://localhost:8080` (Đóng vai trò tiếp nhận sự kiện từ Agent và thực hiện logic đối soát).
- **Elasticsearch:** `http://localhost:9200` (Cơ sở dữ liệu lưu trữ Audit Log phục vụ truy vết).

---

### Bước 2: Cấu hình Hệ thống Tự chữa lành (Auto-Remediation)
Nếu bạn có sẵn hệ thống Ansible AWX / Tower để kiểm thử luồng phục hồi tự động, hãy thực hiện:
1. Mở file `central_service/config.yaml`.
2. Tại mục `remediation:`, cập nhật các giá trị:
   - `ansible_url`: URL của Ansible AWX server.
   - `ansible_token`: Bearer Token xác thực AWX API.
   - `playbook`: Tên hoặc ID của **AWX Job Template** cần trigger (dùng trong endpoint `/api/v2/job_templates/{playbook}/launch/`).
3. Khởi động lại service: `docker compose restart reconciliation` (chạy từ thư mục `deploy/`).

---

### Bước 3: Biên dịch Agent C++ (Trên máy Target)
Tiến hành cài đặt thư viện và biên dịch Agent bằng CMake trên server Production.

```bash
# Cài đặt thư viện phụ thuộc (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev auditd

# Build Agent
cd agent
mkdir -p build && cd build
cmake ..
make
```
Sau khi `make` hoàn tất, file thực thi nhị phân `vdt_agent` sẽ được tạo ra tại thư mục `build/`.

---

### Bước 4: Thiết lập Audit Rules và Khởi chạy Agent
Bản chất Agent C++ hoạt động bằng cách lắng nghe log do Kernel Audit sinh ra. Vì vậy, ta cần nạp các tập luật (rules) giám sát trước khi chạy.

```bash
# 1. Copy file định nghĩa rule vào thư mục cấu hình của OS
sudo cp ../config/vdt-monitor.rules /etc/audit/rules.d/

# 2. Yêu cầu Kernel nạp lại các rule mới
sudo augenrules --load

# 3. Khởi chạy Agent (Bắt buộc dùng sudo để cấp quyền đọc Audit Log)
sudo ./vdt_agent --config ../config/agent.yaml
```
*(Trong môi trường thực tế, thay vì chạy trực tiếp, hãy copy script trong thư mục `systemd/` vào `/etc/systemd/system/` và quản lý bằng `systemctl start vdt-agent` để đảm bảo Agent tự khởi động lại khi sập).*

---

## Cấu trúc Project

```text
Out-of-band-Deployment-Monitor/
├── agent/                              # C++ Agent bắt sự kiện
│   ├── config/                         
│   │   ├── agent.yaml                  # File cấu hình chung
│   │   └── vdt-monitor.rules           # Tập luật giám sát thư mục
│   ├── include/                        # [Thư mục Header files]
│   │   ├── audit_watcher.h
│   │   ├── config.h
│   │   ├── event_queue.h
│   │   ├── event_serializer.h
│   │   ├── event.h
│   │   ├── logger.h
│   │   ├── network_client.h
│   │   └── session_tracker.h
│   ├── src/                            # [Thư mục Source code chính]
│   │   ├── audit_watcher.cpp           # Đọc log liên tục từ /var/log/audit/audit.log
│   │   ├── config.cpp
│   │   ├── event_queue.cpp
│   │   ├── event_serializer.cpp        # Đóng gói JSON
│   │   ├── event.cpp
│   │   ├── logger.cpp
│   │   ├── main.cpp                    # Entrypoint
│   │   ├── network_client.cpp          # Gửi HTTP POST về Central Service
│   │   └── session_tracker.cpp         # Truy vết Session/User thực thi lệnh
│   ├── systemd/                        # Script chạy ngầm Systemd
│   └── CMakeLists.txt                  # Cấu hình build CMake
│
├── central_service/                    # Python FastAPI Backend (Xử lý đối soát)
│   ├── api/
│   │   └── events.py                   # REST Endpoint tiếp nhận Data
│   ├── schemas/
│   │   ├── __init__.py
│   │   └── event_schema.py             # Pydantic schema
│   ├── services/                       # [Business Logic]
│   │   ├── __init__.py
│   │   ├── elasticsearch_client.py     # Đẩy log vi phạm về Elasticsearch
│   │   ├── ip_checker.py
│   │   ├── jenkins_client.py           # Cross-check trạng thái Job Jenkins
│   │   ├── reconciliation_engine.py    # Logic lõi: Phân loại HIGH/WARNING
│   │   └── remediation_client.py       # Tự chữa lành (Gọi API Ansible AWX)
│   ├── .dockerignore
│   ├── config.yaml                     # Cấu hình IP, Token, URLs
│   ├── Dockerfile
│   ├── main.py                         # Khởi chạy FastAPI
│   └── requirements.txt
│
└── deploy/                             # Nhóm cấu hình Infrastructure
    ├── grafana/
    │   ├── dashboards/
    │   │   └── vdt_dashboard.json      # Dashboard giám sát Drift & Alert
    │   └── provisioning/
    │       ├── alerting/
    │       │   └── alerting.yaml       # Cấu hình Webhook Rule
    │       ├── dashboards/
    │       │   └── dashboard.yaml
    │       └── datasources/
    │           └── es.yaml             # Datasource Elasticsearch
    └── docker-compose.yml              # Khởi tạo toàn bộ Monitoring Stack
```

---

## Tiêu chí Đánh giá (E2E)

Hệ thống được thiết kế và kiểm thử hướng tới việc đáp ứng các tiêu chí khắt khe của Đề bài:
- ✅ **Giảm thiểu False Positive:** Xây dựng cơ chế kiểm tra chéo (Cross-check) IP và Jenkins state nhằm hạn chế tối đa cảnh báo nhầm khi Ansible đang deploy hợp lệ.
- ✅ **Low Overhead:** Việc sử dụng cơ chế theo dõi log liên tục (Log Tailing) thay vì quét thư mục (Polling) giúp Agent C++ hoạt động nhẹ nhàng, không gây ảnh hưởng đến hiệu năng của Production Server.
- ✅ **Real-time Alerting:** Luồng xử lý bất đồng bộ (Asynchronous) giúp hệ thống phản hồi nhanh chóng. Thực nghiệm cho thấy cảnh báo đẩy lên Grafana và việc kích hoạt Ansible diễn ra rất mượt mà, hoàn toàn thỏa mãn yêu cầu thời gian < 1 phút.

---
*Dự án thực hiện cho chương trình Viettel Digital Talent.*
