from pathlib import Path
import subprocess,re,json
p=Path(__file__).resolve().parents[1];report={}
for config in ('Debug','Release'):
 elf=next((p/'build'/config).glob('*.elf'))
 nm=subprocess.check_output(['arm-none-eabi-nm',str(elf)],text=True)
 symbols={m[2]:int(m[1],16) for line in nm.splitlines() if (m:=re.match(r'([0-9a-fA-F]+)\s+\w\s+(__board_\w+)',line))}
 output=subprocess.check_output(['arm-none-eabi-size',str(elf)],text=True)
 numbers=list(map(int,output.splitlines()[1].split()[:4]))
 assert [symbols['__board_text'],symbols['__board_data'],symbols['__board_bss']]==numbers[:3],(symbols,numbers)
 assert sum(numbers[:3])==numbers[3]
 report[config]={k.removeprefix('__board_'):v for k,v in symbols.items()};report[config]['dec']=numbers[3]
 print(config, 'PASS: UI linker metrics match arm-none-eabi-size',numbers)
(p/'validation').mkdir(exist_ok=True)
(p/'validation/memory-build.json').write_text(json.dumps(report,indent=2),encoding='utf-8')

