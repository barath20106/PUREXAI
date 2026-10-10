import os, time, requests
from dataclasses import dataclass
from typing import Any

DEFAULT_BASE = 'http://192.168.0.7:8000'

@dataclass
class APIClient:
    base_url: str = DEFAULT_BASE
    timeout: float = 5.0
    def __post_init__(self):
        self.base_url = os.getenv('PUREXAI_API_URL', self.base_url).rstrip('/')
    def get(self, path: str, params=None):
        url = self.base_url + path
        r = requests.get(url, params=params, timeout=self.timeout, headers={'Cache-Control':'no-cache','Pragma':'no-cache'},)
        r.raise_for_status()
        return r.json()

api = APIClient()

def safe_get(path, params=None):
    try:
        return api.get(path, params)
    except Exception as e:
        return None

def get_live_bundle():
    try:
        live = api.get('/api/readings/live')
        status = safe_get('/api/status') or {}
        health = safe_get('/api/sensor-health') or {}
        stats = safe_get('/api/stats') or {}
        return {'ok': True, 'live': live, 'status': status, 'health': health, 'stats': stats, 'error': ''}
    except Exception as e:
        return {'ok': False, 'live': None, 'status': None, 'health': None, 'stats': None, 'error': str(e)}

def history(limit=200):
    data = safe_get('/api/readings/history', {'limit': limit})
    if data is None: return []
    if isinstance(data, list): return data
    for k in ('readings','history','data','items'):
        if isinstance(data, dict) and isinstance(data.get(k), list): return data[k]
    return []

def alerts(active=False):
    data = safe_get('/api/alerts/active' if active else '/api/alerts')
    if data is None: return []
    if isinstance(data, list): return data
    for k in ('alerts','items','data'):
        if isinstance(data, dict) and isinstance(data.get(k), list): return data[k]
    return []
