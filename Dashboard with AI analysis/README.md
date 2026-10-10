# PureXAI — Streamlit Dashboard Integration Guide

This guide explains how the **Streamlit Dashboard** communicates with the **FastAPI Backend** using HTTP REST APIs.

## Critical Architecture Rule

The Streamlit dashboard must **never directly access the SQLite database file or import SQLAlchemy models**. All data, including live readings, historical readings, sensor health, statistics, and alerts, must be retrieved exclusively through the backend's HTTP REST APIs.

## 1. Backend Base URL Configuration

Configure the backend URL using an environment variable.

```python
import os
import requests

BACKEND_URL = os.getenv("BACKEND_URL", "http://localhost:8000")
```

## 2. API Endpoints Reference

### A. Live Sensor Readings

**Endpoint:** `GET /api/readings/live`

Returns the latest sensor reading, individual parameter statuses, overall water-quality status, and device connectivity.

```python
response = requests.get(
    f"{BACKEND_URL}/api/readings/live",
    timeout=10
)
response.raise_for_status()
data = response.json()
```

**Example response:**

```json
{
  "reading": {
    "id": 42,
    "timestamp": "2026-08-29T12:00:00",
    "tds_ppm": 245.3,
    "turbidity_ntu": 0.72,
    "temperature_c": 22.1,
    "tds_status": "Safe",
    "turbidity_status": "Safe",
    "temperature_status": "Safe",
    "overall_status": "Safe",
    "device_id": "esp32-001"
  },
  "device_online": true,
  "last_seen": "2026-08-29T12:00:00",
  "message": "Live"
}
```

### B. Current Water-Quality Status

**Endpoint:** `GET /api/status`

Returns the current water-quality summary for status indicators and dashboard badges.

```python
response = requests.get(
    f"{BACKEND_URL}/api/status",
    timeout=10
)
response.raise_for_status()
status_data = response.json()
```

**Example response:**

```json
{
  "overall_status": "Safe",
  "tds_status": "Safe",
  "turbidity_status": "Safe",
  "temperature_status": "Safe",
  "tds_ppm": 245.3,
  "turbidity_ntu": 0.72,
  "temperature_c": 22.1,
  "timestamp": "2026-08-29T12:00:00",
  "message": "[SAFE] Water quality is SAFE for consumption."
}
```

### C. 24-Hour Statistics

**Endpoint:** `GET /api/stats`

Returns aggregated metrics for dashboard cards, summaries, and statistical visualizations.

```python
response = requests.get(
    f"{BACKEND_URL}/api/stats",
    timeout=10
)
response.raise_for_status()
stats = response.json()
```

**Example response:**

```json
{
  "total_readings": 1250,
  "readings_today": 340,
  "avg_tds_24h": 220.4,
  "avg_turbidity_24h": 0.65,
  "avg_temperature_24h": 21.8,
  "safe_count_24h": 1200,
  "warning_count_24h": 40,
  "unsafe_count_24h": 10,
  "safe_pct_24h": 96.0,
  "warning_pct_24h": 3.2,
  "unsafe_pct_24h": 0.8,
  "active_alerts_count": 0,
  "device_status": "ONLINE"
}
```

### D. Historical Sensor Readings

**Endpoint:** `GET /api/readings/history`

Used to retrieve historical readings for time-series charts and trend analysis.

**Supported query parameters:**

| Parameter | Type | Description |
|---|---|---|
| `page` | Integer | Page number; default is 1 |
| `per_page` | Integer | Records per page; default is 50, maximum is 500 |
| `start_time` | ISO datetime | Optional start of the time range |
| `end_time` | ISO datetime | Optional end of the time range |
| `status` | String | Optional status filter: `Safe`, `Warning`, or `Unsafe` |
| `device_id` | String | Optional device identifier |

**Example:**

```python
params = {
    "page": 1,
    "per_page": 200,
    "start_time": "2026-08-28T00:00:00",
    "end_time": "2026-08-29T23:59:59"
}

response = requests.get(
    f"{BACKEND_URL}/api/readings/history",
    params=params,
    timeout=10
)
response.raise_for_status()
history = response.json()

readings = history.get("data", [])
```

Historical data can be visualized using Streamlit charts, Plotly, or Altair.

### E. Sensor Health Status

**Endpoint:** `GET /api/sensor-health`

Returns the health and connectivity status of individual sensors.

Possible statuses include `ONLINE`, `OFFLINE`, and `ERROR`.

```python
response = requests.get(
    f"{BACKEND_URL}/api/sensor-health",
    timeout=10
)
response.raise_for_status()
health = response.json()
```

**Example response:**

```json
{
  "sensors": [
    {
      "id": 1,
      "device_id": "esp32-001",
      "sensor_name": "tds",
      "status": "ONLINE",
      "last_seen": "2026-08-29T12:00:00",
      "last_value": 245.3,
      "error_count": 0,
      "updated_at": "2026-08-29T12:00:00"
    }
  ],
  "device_online": true
}
```

### F. Alert Management APIs

#### 1. Retrieve Active Unresolved Alerts

**Endpoint:** `GET /api/alerts/active`

Returns alerts that have not yet been resolved.

```python
response = requests.get(
    f"{BACKEND_URL}/api/alerts/active",
    timeout=10
)
response.raise_for_status()

active_alerts = response.json().get("data", [])
```

#### 2. Retrieve Filtered Alert History

**Endpoint:** `GET /api/alerts`

Supports filtering alerts by severity, resolution status, and pagination.

```python
params = {
    "severity": "CRITICAL",
    "is_resolved": True,
    "per_page": 50
}

response = requests.get(
    f"{BACKEND_URL}/api/alerts",
    params=params,
    timeout=10
)
response.raise_for_status()

alert_history = response.json()
```

#### 3. Resolve an Alert

**Endpoint:** `PATCH /api/alerts/{alert_id}/resolve`

Marks an alert as resolved.

```python
alert_id = 5

response = requests.patch(
    f"{BACKEND_URL}/api/alerts/{alert_id}/resolve",
    json={"resolved_by": "operator_admin"},
    timeout=10
)
response.raise_for_status()
```

## 3. Integration Guidelines

- **API-only communication:** All dashboard data must come from the FastAPI backend.
- **Environment-based configuration:** Use `BACKEND_URL` to configure the backend address.
- **Error handling:** Use request timeouts and `raise_for_status()` to handle HTTP errors.
- **Historical visualization:** Use the history endpoint for trends and time-series charts.
- **Sensor monitoring:** Display individual sensor health and device connectivity.
- **Alert management:** Provide interfaces for viewing active alerts, filtering alert history, and resolving alerts.
- **Data validation:** Handle missing readings, unavailable devices, malformed responses, and backend connection failures gracefully.
- **Security:** Never expose credentials, API keys, or other secrets in the repository.

## 4. Running the Dashboard

Ensure the FastAPI backend is running and accessible before starting the Streamlit dashboard.

Set the backend URL if necessary:

```bash
# Windows Command Prompt
set BACKEND_URL=http://localhost:8000

# Windows PowerShell
$env:BACKEND_URL="http://localhost:8000"

# Linux or macOS
export BACKEND_URL=http://localhost:8000
```

Start the Streamlit application:

```bash
streamlit run app.py
```

Replace `app.py` with the actual filename of your Streamlit entry-point file.

## 5. API Documentation

FastAPI commonly provides interactive API documentation at:

- Swagger UI: `http://localhost:8000/docs`
- ReDoc: `http://localhost:8000/redoc`

These URLs work when the backend is running locally with the default configuration.

## Conclusion

The PureXAI dashboard uses a REST API-based architecture to obtain real-time sensor readings, historical data, water-quality status, sensor health, statistical summaries, and alert information from the FastAPI backend. This separation keeps database access within the backend and makes the dashboard easier to maintain and extend.
