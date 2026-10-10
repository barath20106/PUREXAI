import streamlit as st
from common import history_df
st.header('🤖 AI Analysis')
df=history_df(1000)
if df.empty: st.info('AI analysis needs historical sensor readings.'); st.stop()
latest=df.iloc[-1]
tds=float(latest.get('tds_ppm',0) or 0); turb=float(latest.get('turbidity_ntu',0) or 0); temp=float(latest.get('temperature_c',0) or 0)
risk=0
if tds>300:risk+=30
if tds>500:risk+=30
if turb>5:risk+=20
if turb>10:risk+=20
if temp<5 or temp>40:risk+=10
risk=min(100,risk)
st.metric('Water Risk Score',f'{risk}/100')
if risk<30: st.success('Low risk — parameters are currently within the project safe range.')
elif risk<60: st.warning('Moderate risk — monitor the affected parameter.')
else: st.error('High risk — inspect water quality and filter condition.')
st.write({'TDS':tds,'Turbidity':turb,'Temperature':temp})
