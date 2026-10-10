import streamlit as st
from common import history_df
st.header('📈 Historical Data')
df=history_df(1000)
if df.empty: st.warning('No historical readings returned by FastAPI.')
else:
    st.dataframe(df.sort_values('timestamp',ascending=False) if 'timestamp' in df else df,use_container_width=True,hide_index=True)
    cols=[c for c in ['tds_ppm','turbidity_ntu','temperature_c'] if c in df]
    if 'timestamp' in df and cols:
        st.line_chart(df.sort_values('timestamp').set_index('timestamp')[cols])
