import streamlit as st
from api_client import alerts
st.header('🚨 Alert History')
@st.fragment(run_every='3s')
def page():
    a=alerts(False)
    if a: st.dataframe(a,use_container_width=True,hide_index=True)
    else: st.info('No alerts returned.')
page()
