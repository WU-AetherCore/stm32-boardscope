"""板上调试输入回归；读帧缓冲，短暂切换继电器1后恢复关闭。"""
from pathlib import Path
import subprocess,re,time,json
from PIL import Image
elf='build/Debug/STM32F407VET6-CLion-V1.2.elf'
log=[]
def gdb(commands):
    p=Path('validation/step.gdb')
    p.write_text('set pagination off\ntarget remote localhost:61234\nset *(unsigned int*)0xE0042008 = *(unsigned int*)0xE0042008 | 0x1000\n'+commands+'\ndetach\nquit\n',encoding='utf-8')
    r=subprocess.run(['arm-none-eabi-gdb','-q','-batch',elf,'-x',str(p)],capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=15)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)
    return r.stdout

def check(name,page,selection=None,relay=None,limit=None):
    cmd='p astra_page\np astra_selection\np relay1_flag_dat\np Temperature_max\np astra_alloc_failures\np board_ui_heartbeat\np board_oled_status\np board_ui_status'
    cmd+=f'\ndump binary memory validation/astra-{name}.bin astra_framebuffer astra_framebuffer+1024'
    out=gdb(cmd)
    values=[int(x) for x in re.findall(r'\$\d+ = (\d+)',out)]
    assert len(values)==8,(name,out)
    assert values[0]==page,(name,values)
    if selection is not None:assert values[1]==selection,(name,values)
    if relay is not None:assert values[2]==relay,(name,values)
    if limit is not None:assert values[3]==limit,(name,values)
    assert values[4]==0 and values[6:]==[0,0],(name,values)
    b=Path(f'validation/astra-{name}.bin').read_bytes();im=Image.new('RGB',(128,64),'#071018')
    for y in range(64):
        for x in range(128):
            if b[y//8*128+x]&(1<<(y%8)):im.putpixel((x,y),(235,255,255))
    im.resize((768,384)).save(f'validation/astra-{name}.png')
    log.append(dict(test=name,values=values));print('PASS',name,values,flush=True)

def move(turn=0,ok=True):
    gdb(f'set board_debug_turn = {turn}\nset board_debug_ok = {int(ok)}')
    time.sleep(.6)
try:
    check('home',1,0)
    move();check('sensor',2,0)
    move();move(1);check('relay',3,0,relay=0)
    move();check('relay-cancel-default',12,1,relay=0)
    move();check('relay-cancelled',3,0,relay=0)
    move();move(-1);check('relay-on',3,0,relay=1)
    move();move(-1);check('relay-off',3,0,relay=0)
    move(5);move(1);check('uart',4,0)
    move();move(1,False);move();check('uart2-selected',4,0)
    move(2);check('uart-hex',4,2)
    move(17);move(1);check('flash',5,0)
    move(3);move(1,False);move();check('flash-address16',5,3)
    move(7);check('flash-address0',5,10)
    move(1);move(1);check('keys',6,0)
    move();move(1);check('system',7,0)
    move(4);check('can',10,0)
    move(6);move(1);check('gpio',11,0)
    move(5);move(1);move(1);check('memory',8,0)
    move(10,False);check('memory-heap',8,10)
    move(5);move(1);check('settings',9,0)
    move(1);move(1,False);move();check('limit46',9,1,limit=46)
    move();move(-1,False);move();check('limit45',9,1,limit=45)
    move(3);move(1,False);check('final-home',1,0,relay=0,limit=45)
finally:
    gdb('set relay1_flag_dat = 0\nset relay2_flag_dat = 0\nset relay3_flag_dat = 0\nset relay4_flag_dat = 0\nset relay5_flag_dat = 0\nset Temperature_max = 45')
    Path('validation/astra-navigation.json').write_text(json.dumps(log,indent=2),encoding='utf-8')
