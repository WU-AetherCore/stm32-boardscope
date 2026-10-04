"""协议回归：丢行、乱序、重复编号、非法 JSON 和重启处理。"""
import sys,json,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from boardscope_protocol import SnapshotAssembler,FIELDS

def fixture(seq=1):
    kinds=['begin','sensor','relay','flash','memory','sections','heap','ui']+['key']*3+['can','can_data']+['uart']*6+['uart_state']+['gpio']*5+['end']
    rows=[dict(s=seq,t=k,v=[0]*len(FIELDS[k])) for k in kinds]
    rows[0]['v']=[1,1000,168000000,1000]
    for i in range(3): rows[8+i]['v'][0]=i+1
    for i in range(6): rows[13+i]['v'][0]=i+1
    for i in range(5): rows[20+i]['v'][0]=i
    rows[-1]['v']=[0,26]
    return rows

def parse(rows):
    a=SnapshotAssembler();results=[]
    for row in rows:
        r=a.feed(json.dumps(row))
        if r: results.append(r)
    return results

class ProtocolTest(unittest.TestCase):
    def test_complete_and_restart(self):
        self.assertEqual(len(parse(fixture(100)+fixture(1))),2)
    def test_missing_line(self):
        f=fixture();del f[12];self.assertEqual(parse(f),[])
    def test_wrong_order(self):
        f=fixture();f[5],f[6]=f[6],f[5];self.assertEqual(parse(f),[])
    def test_duplicate_port(self):
        f=fixture();f[14]['v'][0]=1;self.assertEqual(parse(f),[])
    def test_bad_data(self):
        a=SnapshotAssembler()
        for line in ['STATUS','[]','null','{"s":true,"t":"begin","v":[]}','{"s":1,"t":[],"v":[]}']:
            self.assertIsNone(a.feed(line))
        self.assertEqual(a.bad_lines,5)
    def test_packet_capacity(self):
        for r in fixture(0xffffffff):
            r['v']=[0xffffffff]*len(r['v'])
            if r['t']=='can_data': r['v']=[255]*8
            encoded=json.dumps(r,separators=(',',':')).encode()+b'\r\n'
            self.assertLess(len(encoded),128,(r['t'],len(encoded)))

if __name__=='__main__': unittest.main()
