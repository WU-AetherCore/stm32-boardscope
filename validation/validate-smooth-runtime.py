from pathlib import Path
import serial,time,json,sys
sys.path.insert(0,'tools')
from boardscope_protocol import SnapshotAssembler
a=SnapshotAssembler();frames=[];lines=[]
with serial.Serial('COM6',115200,timeout=.3) as port:
 port.reset_input_buffer();until=time.monotonic()+15
 while time.monotonic()<until:
  line=port.readline().decode('utf-8',errors='replace').strip()
  if not line:continue
  lines.append(line);frame=a.feed(line)
  if frame:frames.append(frame)
assert len(frames)>=10,len(frames)
intervals=[b[0]['v'][1]-a[0]['v'][1] for a,b in zip(frames,frames[1:])]
assert all(990<=v<=1020 for v in intervals),intervals
assert all(f[7]['v'][:2]==[0,0] for f in frames)
assert all(f[2]['v'][:5]==[0]*5 for f in frames)
assert all(all(r['v'][3:]==[0]*4 for r in f[13:19]) for f in frames)
valid=sum(f[1]['v'][2]==1 for f in frames);assert valid>=len(frames)-2
rates=[round(1000*(b[7]['v'][2]-a[7]['v'][2])/(b[0]['v'][1]-a[0]['v'][1]),2) for a,b in zip(frames,frames[1:])]
report=dict(frames=len(frames),interval_ms=intervals,ui_iterations_per_second=rates,valid_sensor_frames=valid,heap=frames[-1][6]['v'],ui=frames[-1][7]['v'],sensor=frames[-1][1]['v'],bad_lines=a.bad_lines,last=frames[-1])
Path('validation/smooth-final-runtime.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
Path('validation/smooth-final-runtime.log').write_text('\n'.join(lines),encoding='utf-8')
print(json.dumps({k:v for k,v in report.items() if k!='last'}))
