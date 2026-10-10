PUREXAI FINAL STREAMLIT DASHBOARD

1. Install:
   py -m pip install -r requirements.txt

2. IMPORTANT: edit api_client.py backend IP changes.
   Current backend: http://192.168.0.7:8000
   Or set environment variable:
   $env:PUREXAI_API_URL="http://192.168.0.7:8000"

3. Start:
   py -m streamlit run app.py --server.address 0.0.0.0 --server.port 8501

4. Open :
   http://127.0.0.1:8501

5. Before starting Streamlit, test from browser:
   http://192.168.0.7:8000/api/readings/live
   It must return JSON with current ESP32 data.

REAL-TIME FIX:
- No Streamlit cache is used for API calls.
- Dashboard requests FastAPI every 2 seconds using st.fragment.
- HTTP requests include no-cache headers.
- All pages read the same FastAPI source.

NETWORK:
ESP32 -> router -> 192.168.0.7:8000 (backend) -> dashboard
All devices must be on the same LAN, and Windows Firewall on backend laptop must allow TCP 8000.
