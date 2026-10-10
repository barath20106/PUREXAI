import streamlit as st
from common import history_df
st.header('📄 Reports')
df=history_df(5000)
if df.empty: st.info('No data to report.'); st.stop()
summary=df.describe(include='all').transpose()
st.dataframe(summary,use_container_width=True)
st.download_button('⬇️ Download Historical CSV',df.to_csv(index=False).encode('utf-8'),'purexai_historical_data.csv','text/csv')
st.download_button('⬇️ Download Summary CSV',summary.to_csv().encode('utf-8'),'purexai_summary.csv','text/csv')
