import pandas as pd
import streamlit as st
from api_client import get_live_bundle, history, alerts

def flatten_live():
    b=get_live_bundle(); live=b.get('live') or {}; r=live.get('reading',live) if isinstance(live,dict) else {}
    return b,r

def history_df(limit=500):
    rows=history(limit)
    if not rows: return pd.DataFrame()
    df=pd.DataFrame(rows)
    # Normalize common backend field names
    ren={}
    for a,b in [('tds','tds_ppm'),('turbidity','turbidity_ntu'),('temperature','temperature_c'),('water_status','overall_status')]:
        if a in df.columns and b not in df.columns: ren[a]=b
    df=df.rename(columns=ren)
    if 'timestamp' in df.columns: df['timestamp']=pd.to_datetime(df['timestamp'],errors='coerce')
    for c in ['tds_ppm','turbidity_ntu','temperature_c']:
        if c in df.columns: df[c]=pd.to_numeric(df[c],errors='coerce')
    return df

def status_badge(status):
    s=str(status or 'UNKNOWN').upper()
    if s=='SAFE': st.success('🟢 SAFE')
    elif s=='WARNING': st.warning('🟡 WARNING')
    elif s=='UNSAFE': st.error('🔴 UNSAFE')
    else: st.info(f'⚪ {s}')

@st.fragment(run_every='2s')
def live_cards():
    b,r=flatten_live()
    if not b['ok']:
        st.error('Unable to retrieve live data from FastAPI: '+b['error']); return
    def g(*ks,d=None):
        for k in ks:
            if r.get(k) is not None:return r[k]
        return d
    c1,c2,c3,c4=st.columns(4)
    c1.metric('TDS',f"{float(g('tds_ppm','tds',d=0)):.2f} ppm")
    c2.metric('Turbidity',f"{float(g('turbidity_ntu','turbidity',d=0)):.2f} NTU")
    c3.metric('Temperature',f"{float(g('temperature_c','temperature',d=0)):.2f} °C")
    c4.metric('Status',str(g('overall_status','water_status',d='Unknown')).upper())
    status_badge(g('overall_status','water_status',d='Unknown'))
    st.caption('Fresh API request every 2 seconds. Browser refresh is not required.')
