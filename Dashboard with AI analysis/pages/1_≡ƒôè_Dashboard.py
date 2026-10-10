import streamlit as st
from api_client import api, get_live_bundle

st.set_page_config(
    page_title="PUREXAI Dashboard",
    page_icon="📊",
    layout="wide"
)

st.header("📊 PUREXAI Live Dashboard")
st.caption("ESP32 → FastAPI → Streamlit")


def get_value(data, *keys, default=None):

    if not isinstance(data, dict):
        return default

    for key in keys:
        value = data.get(key)

        if value is not None:
            return value

    return default


def calculate_status(tds, turbidity):

    try:
        tds = float(tds)
        turbidity = float(turbidity)
    except (TypeError, ValueError):
        return "UNKNOWN"

    if tds > 500 or turbidity > 10:
        return "UNSAFE"

    if tds > 300 or turbidity > 5:
        return "WARNING"

    return "SAFE"


@st.fragment(run_every="2s")
def dashboard():

    try:
        bundle = get_live_bundle()

    except Exception as e:

        st.error(f"FastAPI error: {e}")
        return

    if not bundle.get("ok"):

        st.error(
            f"FastAPI connection failed: "
            f"{bundle.get('error', 'Unknown error')}"
        )

        return

    live = bundle.get("live") or {}
    status_data = bundle.get("status") or {}

    if not isinstance(live, dict):
        live = {}

    if not isinstance(status_data, dict):
        status_data = {}

    reading = live.get("reading", live)

    if not isinstance(reading, dict):
        reading = {}

    # ------------------------------------------------------------
    # VALUES
    # ------------------------------------------------------------

    tds = get_value(
        reading,
        "tds_ppm",
        "tds",
        default=None
    )

    turbidity = get_value(
        reading,
        "turbidity_ntu",
        "turbidity",
        default=None
    )

    temperature = get_value(
        reading,
        "temperature_c",
        "temperature",
        default=None
    )

    timestamp = get_value(
        reading,
        "timestamp",
        default="—"
    )

    device_id = get_value(
        reading,
        "device_id",
        default="PUREXAI_ESP32_001"
    )

    # ------------------------------------------------------------
    # STATUS FROM FASTAPI
    # ------------------------------------------------------------

    status = get_value(
        reading,
        "overall_status",
        "water_status",
        default=None
    )

    if status is None:

        status = get_value(
            status_data,
            "overall_status",
            "water_status",
            "status",
            default=None
        )

    if status:

        status = str(status).strip().upper()

    if status not in ["SAFE", "WARNING", "UNSAFE"]:

        status = calculate_status(
            tds,
            turbidity
        )

    # ------------------------------------------------------------
    # CONNECTION
    # ------------------------------------------------------------

    st.success(
        f"🟢 FastAPI Connected — {api.base_url}"
    )

    # ------------------------------------------------------------
    # STATUS
    # ------------------------------------------------------------

    if status == "SAFE":

        st.success("🟢 SAFE — Water quality is normal")

    elif status == "WARNING":

        st.warning("🟡 WARNING — Check water quality")

    elif status == "UNSAFE":

        st.error("🔴 UNSAFE — Immediate attention required")

    else:

        st.info("⚪ UNKNOWN")

    # ------------------------------------------------------------
    # METRICS
    # ------------------------------------------------------------

    c1, c2, c3, c4 = st.columns(4)

    try:
        tds_text = f"{float(tds):.2f} ppm"
    except:
        tds_text = "—"

    try:
        turb_text = f"{float(turbidity):.2f} NTU"
    except:
        turb_text = "—"

    try:
        temp_text = f"{float(temperature):.2f} °C"
    except:
        temp_text = "—"

    c1.metric("💧 TDS", tds_text)
    c2.metric("🌊 Turbidity", turb_text)
    c3.metric("🌡️ Temperature", temp_text)
    c4.metric("🚦 Status", status)

    st.divider()

    c1, c2, c3 = st.columns(3)

    c1.write(f"**Device:** {device_id}")
    c2.write(f"**Last reading:** {timestamp}")
    c3.write("**🔄 Updating:** Every 2 seconds")

    # ------------------------------------------------------------
    # DEBUG
    # ------------------------------------------------------------

    with st.expander("🔍 API Response"):

        st.json(live)

        st.write(f"Dashboard status: **{status}**")


dashboard()
