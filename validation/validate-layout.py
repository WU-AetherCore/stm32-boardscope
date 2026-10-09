from pathlib import Path
import subprocess,time,json
from PIL import Image
elf='build/Debug/STM32F407VET6-CLion-V1.2.elf'
def gdb(commands):
 p=Path('validation/layout.gdb');p.write_text('set pagination off\ntarget remote localhost:61234\nset *(unsigned int*)0xE0042008 = *(unsigned int*)0xE0042008 | 0x1000\n'+commands+'\ndetach\nquit\n')
 r=subprocess.run(['arm-none-eabi-gdb','-q','-batch',elf,'-x',str(p)],capture_output=True,text=True,timeout=15)
 if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 return r.stdout
def capture(name):
 gdb(f'dump binary memory validation/{name}.bin astra_framebuffer astra_framebuffer+1024')
 b=Path(f'validation/{name}.bin').read_bytes();im=Image.new('RGB',(128,64),'#071018')
 for y in range(64):
  for x in range(128):
   if b[y//8*128+x]&(1<<(y%8)):im.putpixel((x,y),(235,255,255))
 im.resize((768,384)).save(f'validation/{name}.png')
try:
 gdb("set '(anonymous namespace)::edit' = 13\nset '(anonymous namespace)::editValue' = 16777200")
 time.sleep(.6);capture('smooth-max-address')
 gdb("set '(anonymous namespace)::edit' = 16\nset '(anonymous namespace)::editValue' = 100")
 time.sleep(.6);capture('smooth-brightness')
 print(gdb('p astra_ui_frame_us\np astra_ui_max_us\np astra_display_frame_us\np astra_display_max_us\np astra_display_errors\np astra_display_transfers\np astra_display_skipped'))
finally:gdb("set '(anonymous namespace)::edit' = 0")
