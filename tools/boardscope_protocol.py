"""BoardScope JSON 行快照组装器；只提交完整的 26 行快照。"""
import json

FIELDS = {
    'begin': ('协议版本', '运行时间(ms)', '系统时钟(Hz)', '上报周期(ms)'),
    'sensor': ('温度(℃)', '湿度(%)', '最近采样有效', '成功采样距今(ms)', '温度上限(℃)', '温度报警', '电压预设(mV)', '电压实测有效'),
    'relay': ('继电器1命令', '继电器2命令', '继电器3命令', '继电器4命令', '继电器5命令', '蜂鸣器命令', '报警使能'),
    'flash': ('通信有效', 'JEDEC ID', '容量(B)', '状态寄存器'),
    'memory': ('FLASH已用(B)', 'FLASH总量(B)', 'RAM已用(B)', 'RAM总量(B)', 'CCM已用(B)', 'CCM总量(B)'),
    'sections': ('text(B)', 'data(B)', 'bss(B)', 'dec(B)'),
    'heap': ('堆总量(B)', '堆空闲(B)', '历史最小空闲(B)', '任务数', '通信任务最小栈余量(B)'),
    'ui': ('OLED状态码', 'UI状态码', 'UI心跳', 'UI最小栈余量(B)'),
    'key': ('按键编号', '短按次数', '长按次数', '双击次数'),
    'can': ('接收次数', '最后ID', '最后长度', 'HAL错误码'),
    'can_data': tuple('数据字节'+str(i) for i in range(8)),
    'uart': ('串口编号', '接收字节', '发送完成字节', 'TX队列拒绝包', 'RX队列丢包', '错误次数', '发送失败次数'),
    'uart_state': tuple('UART'+str(i)+'排队/回显编码' for i in range(1,7)),
    'gpio': ('端口编号(A=0)', 'IDR输入寄存器', 'ODR输出寄存器'),
    'end': ('错过上报周期数', '记录总数'),
}

class SnapshotAssembler:
    def __init__(self):
        self.sequence = None
        self.rows = []
        self.bad_lines = 0

    def feed(self, line):
        try:
            record = json.loads(line)
            seq, kind, values = record['s'], record['t'], record['v']
            if type(seq) is not int or not 0 <= seq <= 0xffffffff:
                raise ValueError('sequence')
            if kind not in FIELDS or not isinstance(values, list) or len(values) != len(FIELDS[kind]):
                raise ValueError('shape')
            if any(type(v) is not int or not 0 <= v <= 0xffffffff for v in values):
                raise ValueError('values')
        except (ValueError, KeyError, TypeError):
            self.bad_lines += 1
            return None
        if kind == 'begin':
            self.sequence, self.rows = seq, []
        if seq != self.sequence:
            return None
        self.rows.append(record)
        if len(self.rows) > 26:
            self.sequence, self.rows = None, []
            return None
        if kind == 'end':
            result = self.rows
            self.sequence, self.rows = None, []
            expected = ['begin','sensor','relay','flash','memory','sections','heap','ui',
                        'key','key','key','can','can_data'] + ['uart']*6 + ['uart_state'] + ['gpio']*5 + ['end']
            if len(result) != 26 or [r['t'] for r in result] != expected or values[1] != 26 or result[0]['v'][0] != 1:
                return None
            if [r['v'][0] for r in result[8:11]] != [1,2,3] or [r['v'][0] for r in result[13:19]] != list(range(1,7)) or [r['v'][0] for r in result[20:25]] != list(range(5)):
                return None
            return result
        return None
