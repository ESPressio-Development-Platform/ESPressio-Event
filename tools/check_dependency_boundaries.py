"""Enforce the Event production DAG and active predecessor eradication."""
import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
allowed={'System','Primitive','Task','Threads','Timing','Serializable'}
manifest=json.loads((root/'library.json').read_text())
actual={d['name'].removeprefix('ESPressio-') for d in manifest['dependencies']}
assert actual==allowed,(actual,allowed)
forbidden=('ESPNow','Socket','Command','State','Security','Mesh','Radio','Adapter','Observable')
for path in (root/'src').rglob('*'):
    if not path.is_file(): continue
    for include in re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)',path.read_text(),re.M):
        assert not include.startswith(tuple('ESPressio_'+p for p in forbidden)),(path,include)
predecessors=('EventManager','EventDispatcher','EventReceiver','EventTransportManager','IEventTransport','EventThreadBase','PrecisionEventThread','ThreadEventBridges','EventTypeKey')
for folder in ('src','examples','.github'):
    for path in (root/folder).rglob('*'):
        if path.is_file():
            text=path.read_text()
            for old in predecessors: assert old not in text,(path,old)
print('Event DAG and active predecessor checks passed')
