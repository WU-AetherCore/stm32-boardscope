"""中文桌面串口监控窗口。安装：python -m pip install -r tools/requirements.txt。"""
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk
from boardscope_protocol import FIELDS, SnapshotAssembler
try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit('请先运行 python -m pip install -r tools/requirements.txt')

class Monitor:
    def __init__(self, root):
        self.root = root
        root.title('STM32 BoardScope · 板载状态监测台')
        root.geometry('1120x760')
        self.events = queue.Queue(maxsize=1024)
        self.stop = threading.Event()
        self.worker = None
        self.last_frame = 0
        self.status = tk.StringVar(value='选择串口后连接；115200 / 8N1 / 无流控')
        bar = ttk.Frame(root, padding=10); bar.pack(fill='x')
        self.port = tk.StringVar(value='COM6')
        self.combo = ttk.Combobox(bar, textvariable=self.port, values=[p.device for p in list_ports.comports()], width=18)
        self.combo.pack(side='left')
        ttk.Button(bar, text='刷新串口', command=self.refresh_ports).pack(side='left', padx=6)
        self.button = ttk.Button(bar, text='连接', command=self.toggle); self.button.pack(side='left')
        ttk.Label(bar, textvariable=self.status).pack(side='left', padx=16)
        tabs = ttk.Notebook(root); tabs.pack(fill='both', expand=True, padx=10, pady=8)
        panel = ttk.Frame(tabs); tabs.add(panel, text='实时状态')
        self.tree = ttk.Treeview(panel, columns=('field','value'), show='tree headings')
        self.tree.heading('#0',text='模块'); self.tree.heading('field',text='数据项'); self.tree.heading('value',text='当前值')
        self.tree.column('#0',width=220); self.tree.column('field',width=310); self.tree.column('value',width=390)
        scroll = ttk.Scrollbar(panel, command=self.tree.yview); self.tree.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right',fill='y'); self.tree.pack(fill='both',expand=True)
        raw_panel = ttk.Frame(tabs); tabs.add(raw_panel,text='原始串口数据')
        self.raw = tk.Text(raw_panel, wrap='none', font=('Consolas',10)); self.raw.pack(fill='both',expand=True)
        ttk.Label(root, text='继电器为软件命令；GPIO 为寄存器采样。无 ADC 电压实测。UART TX 统计为 DMA 完成字节。',padding=8).pack(fill='x')
        root.protocol('WM_DELETE_WINDOW',self.close)
        root.after(100,self.poll)

    def refresh_ports(self):
        self.combo['values'] = [p.device for p in list_ports.comports()]

    def toggle(self):
        if self.worker and self.worker.is_alive():
            self.stop.set(); self.button['state']='disabled'
            return
        self.stop = threading.Event(); self.last_frame=0
        port = self.port.get().strip()
        self.worker = threading.Thread(target=self.read_port,args=(port,self.stop),daemon=True)
        self.worker.start(); self.button['text']='断开'; self.combo['state']='disabled'

    def read_port(self, port, stop):
        assembler = SnapshotAssembler()
        def emit(item):
            try: self.events.put_nowait(item)
            except queue.Full: pass  # 窗口繁忙时丢弃显示消息，禁止阻塞串口读取。
        try:
            with serial.Serial(port,115200,timeout=0.2,write_timeout=0.5) as stream:
                emit(('status',f'{port} 已连接，等待完整快照'))
                pending = bytearray()
                while not stop.is_set():
                    pending.extend(stream.read(max(1,min(stream.in_waiting,4096))))
                    while b'\n' in pending:
                        line, _, pending = pending.partition(b'\n')
                        text = line.decode('utf-8',errors='replace').strip()
                        emit(('raw',text))
                        snapshot = assembler.feed(text)
                        if snapshot: emit(('frame',snapshot))
                    if len(pending)>8192:
                        pending.clear(); emit(('status','收到超长无换行数据，等待下一组快照'))
        except (serial.SerialException, OSError, ValueError) as error:
            emit(('status',f'连接失败/中断：{error}；请关闭其他串口助手后重连'))
        finally:
            emit(('closed',None))

    def display(self, frame):
        for record in frame:
            kind, values = record['t'], record['v']
            suffix = str(values[0]) if kind in ('key','uart','gpio') else ''
            group = kind + suffix
            title = {'begin':'系统','sensor':'温湿度','relay':'继电器','flash':'SPI Flash','memory':'内存占用',
                     'sections':'链接段','heap':'FreeRTOS','ui':'OLED / UI','key':'普通按键','can':'CAN',
                     'can_data':'CAN 数据','uart':'串口','uart_state':'串口队列 / 回显','gpio':'GPIO','end':'上报状态'}[kind]
            if suffix: title += ' ' + (chr(65+values[0]) if kind=='gpio' else suffix)
            if not self.tree.exists(group): self.tree.insert('', 'end',iid=group,text=title,open=True)
            for i,(field,value) in enumerate(zip(FIELDS[kind],values)):
                shown = str(value)
                if kind=='uart_state': shown=f'排队 {value & 15} 包；回显 '+('开启' if value & 16 else '关闭')
                if (kind=='flash' and i in (1,3)) or kind in ('gpio','can_data') and i>0 or kind=='can' and i==1: shown=f'0x{value:X}'
                if kind=='sensor' and i==3 and value==0xffffffff: shown='尚无成功采样'
                node=f'{group}:{i}'
                if self.tree.exists(node): self.tree.item(node,values=(field,shown))
                else: self.tree.insert(group,'end',iid=node,values=(field,shown))
        self.last_frame=time.monotonic()
        self.status.set(f'{self.port.get()} · 快照 #{frame[0]["s"]} · 运行 {frame[0]["v"][1]/1000:.1f} 秒 · 26/26 行完整')

    def poll(self):
        for _ in range(300):
            try: kind,value=self.events.get_nowait()
            except queue.Empty: break
            if kind=='frame': self.display(value)
            elif kind=='raw':
                self.raw.insert('end',value+'\n')
                if int(self.raw.index('end-1c').split('.')[0])>1500: self.raw.delete('1.0','501.0')
                self.raw.see('end')
            elif kind=='status': self.status.set(value)
            elif kind=='closed':
                self.button.configure(text='连接',state='normal'); self.combo['state']='normal'; self.last_frame=0
        if self.last_frame and time.monotonic()-self.last_frame>3:
            self.status.set('超过 3 秒没有完整快照；显示值已过期，请检查连接')
        self.root.after(100,self.poll)

    def close(self):
        self.stop.set()
        if self.worker: self.worker.join(timeout=1)
        self.root.destroy()

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='STM32 BoardScope 中文监控窗口')
    parser.add_argument('--port',default='COM6')
    parser.add_argument('--connect',action='store_true')
    args=parser.parse_args()
    root=tk.Tk(); monitor=Monitor(root); monitor.port.set(args.port)
    if args.connect: monitor.toggle()
    root.mainloop()
