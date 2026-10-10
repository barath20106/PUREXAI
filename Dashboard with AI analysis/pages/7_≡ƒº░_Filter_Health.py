import streamlit as st
from common import history_df
st.header('🧰 Filter Health')
df=history_df(500)
if df.empty: st.info('No history available for filter-health estimation.'); st.stop()
score=100
if 'tds_ppm' in df: score-=min(40,max(0,(df['tds_ppm'].tail(20).mean()-100)/10))
if 'turbidity_ntu' in df: score-=min(50,max(0,df['turbidity_ntu'].tail(20).mean()*5))
score=max(0,min(100,score))
st.metric('Estimated Filter Health',f'{score:.0f}%')
st.progress(int(score))
if score>=80: st.success('Filter condition: GOOD')
elif score>=50: st.warning('Filter condition: ATTENTION')
else: st.error('Filter condition: MAINTENANCE REQUIRED')
