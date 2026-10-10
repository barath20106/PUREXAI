import streamlit as st
from common import live_cards, history_df
st.header('💧 Water Quality')
live_cards()
df=history_df(500)
if not df.empty:
    for c,label in [('tds_ppm','TDS (ppm)'),('turbidity_ntu','Turbidity (NTU)'),('temperature_c','Temperature (°C)')]:
        if c in df:
            st.subheader(label); st.line_chart(df.set_index('timestamp')[c] if 'timestamp' in df else df[c])
