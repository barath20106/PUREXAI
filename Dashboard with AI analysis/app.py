import streamlit as st
from api_client import api, get_live_bundle

st.set_page_config(
    page_title="PUREXAI Water Quality",
    page_icon="💧",
    layout="wide",
    initial_sidebar_state="expanded"
)

st.title("💧 PUREXAI Water Quality Dashboard")
st.caption("ESP32 → Devanand FastAPI → Jai Surya Streamlit")


def get_value(data, *keys, default=None):
    """Get the first non-null value from a dictionary."""
    if not isinstance(data, dict):
        return default

    for key in keys:
        value = data.get(key)
        if value is not None:
            return value

    return default


def calculate_status(tds, turbidity, temperature):
    """
    Dashboard fallback status calculation.

    This is used only when FastAPI does not provide a valid status.
    """

    try:
        tds = float(tds)
        turbidity = float(turbidity)
        temperature = float(temperature)
    except (TypeError, ValueError):
        return "UNKNOWN"

    # Sensor error checks
    if tds < 0 or turbidity < 0:
        return "UNKNOWN"

    # UNSAFE
    if tds > 500 or turbidity > 10:
        return "UNSAFE"

    # WARNING
    if tds > 300 or turbidity > 5:
        return "WARNING"

    # SAFE
    return "SAFE"


def normalize_status(status, tds, turbidity, temperature):
    """
    Use FastAPI status when it is valid.

    If FastAPI sends Unknown/empty status, calculate it from
    the actual sensor values.
    """

    if status is not None:
        status = str(status).strip().upper()

        if status in ["SAFE", "WARNING", "UNSAFE"]:
            return status

    return calculate_status(tds, turbidity, temperature)


@st.fragment(run_every="2s")
def live_dashboard():

    try:
        bundle = get_live_bundle()
    except Exception as e:
        st.error(f"Unable to contact FastAPI: {e}")
        return

    if not bundle.get("ok", False):
        st.error(
            f"FastAPI connection failed: "
            f"{bundle.get('error', 'Unknown error')}"
        )

        st.info(
            "Make sure Devanand's FastAPI is running at "
            f"{api.base_url} and both laptops are connected "
            "to the same Wi-Fi network."
        )
        return

    # ------------------------------------------------------------
    # LIVE RESPONSE
    # ------------------------------------------------------------

    live = bundle.get("live") or {}
    status_response = bundle.get("status") or {}

    if not isinstance(live, dict):
        live = {}

    if not isinstance(status_response, dict):
        status_response = {}

    # FastAPI normally returns:
    #
    # {
    #   "reading": {
    #       ...
    #   }
    # }
    #
    reading = live.get("reading", live)

    if not isinstance(reading, dict):
        reading = {}

    # ------------------------------------------------------------
    # SENSOR VALUES
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
    # STATUS
    # ------------------------------------------------------------

    fastapi_status = get_value(
        reading,
        "overall_status",
        "water_status",
        default=None
    )

    if fastapi_status is None:
        fastapi_status = get_value(
            status_response,
            "overall_status",
            "water_status",
            "status",
            default=None
        )

    overall = normalize_status(
        fastapi_status,
        tds,
        turbidity,
        temperature
    )

    # ------------------------------------------------------------
    # CONNECTION
    # ------------------------------------------------------------

    st.success(
        f"🟢 FastAPI Connected — {api.base_url}"
    )

    # ------------------------------------------------------------
    # STATUS BANNER
    # ------------------------------------------------------------

    if overall == "SAFE":
        st.success("🟢 WATER STATUS: SAFE")

    elif overall == "WARNING":
        st.warning("🟡 WATER STATUS: WARNING")

    elif overall == "UNSAFE":
        st.error("🔴 WATER STATUS: UNSAFE")

    else:
        st.info("⚪ WATER STATUS: UNKNOWN")

    # ------------------------------------------------------------
    # SENSOR CARDS
    # ------------------------------------------------------------

    c1, c2, c3, c4 = st.columns(4)

    # TDS
    if tds is not None:
        try:
            c1.metric(
                "💧 TDS",
                f"{float(tds):.2f} ppm"
            )
        except (TypeError, ValueError):
            c1.metric("💧 TDS", "—")
    else:
        c1.metric("💧 TDS", "—")

    # Turbidity
    if turbidity is not None:
        try:
            c2.metric(
                "🌊 Turbidity",
                f"{float(turbidity):.2f} NTU"
            )
        except (TypeError, ValueError):
            c2.metric("🌊 Turbidity", "—")
    else:
        c2.metric("🌊 Turbidity", "—")

    # Temperature
    if temperature is not None:
        try:
            c3.metric(
                "🌡️ Temperature",
                f"{float(temperature):.2f} °C"
            )
        except (TypeError, ValueError):
            c3.metric("🌡️ Temperature", "—")
    else:
        c3.metric("🌡️ Temperature", "—")

    # Status
    c4.metric(
        "🚦 Water Status",
        overall
    )

    # ------------------------------------------------------------
    # DETAILS
    # ------------------------------------------------------------

    st.divider()

    d1, d2, d3 = st.columns(3)

    d1.write(f"**Device ID:** {device_id}")
    d2.write(f"**Last reading:** {timestamp}")
    d3.write("**🔄 Live refresh:** Every 2 seconds")

    # ------------------------------------------------------------
    # DEBUG INFORMATION
    # ------------------------------------------------------------

    with st.expander("🔍 Live API Data"):

        st.write("FastAPI live response:")

        st.json(live)

        st.write("FastAPI status response:")

        st.json(status_response)

        st.write(
            f"Dashboard status calculation: **{overall}**"
        )


live_dashboard()