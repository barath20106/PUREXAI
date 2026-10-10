import streamlit as st
from common import history_df
st.header('💡 Maintenance')
df=history_df(500)
if df.empty: st.info('No data available.'); st.stop()
issues=[]
if 'tds_ppm' in df and df['tds_ppm'].tail(10).mean()>300: issues.append('Inspect filter: TDS is elevated.')
if 'turbidity_ntu' in df and df['turbidity_ntu'].tail(10).mean()>5: issues.append('Inspect/clean filter: turbidity is elevated.')
if issues:
    for x in issues: st.warning(x)
else: st.success('No immediate maintenance issue detected from current history.')
st.markdown('### Recommended routine checks')
st.write('• Check sensor probes and wiring • Clean turbidity/TDS probes • Verify DS18B20 contact • Inspect filter cartridge • Calibrate sensors when required')
