import streamlit as st
from api_client import safe_get
st.header('🔧 Sensor Health')
@st.fragment(run_every='3s')
def page():
    h=safe_get('/api/sensor-health')
    if h is None: st.error('Cannot retrieve sensor health.'); return
    if isinstance(h,dict) and 'sensors' in h: h=h['sensors']
    if isinstance(h,dict):
        cols=st.columns(min(5,max(1,len(h))))
        for i,(k,v) in enumerate(h.items()): cols[i%len(cols)].metric(str(k).replace('_',' ').title(), 'OK' if v is True else str(v))
    st.json(h)
page()
