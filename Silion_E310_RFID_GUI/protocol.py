from dataclasses import dataclass

@dataclass
class Tag:
    epc: str
    rssi: int
    antenna: int
    frequency_khz: int
    reader_time_ms: int

def parse_line(line: str):
    line = line.strip()
    if line.startswith('TAG,'):
        d = {}
        for part in line.split(',')[1:]:
            if '=' in part:
                k, v = part.split('=', 1)
                d[k.upper()] = v
        try:
            return 'TAG', Tag(d['EPC'].upper(), int(d['RSSI']), int(d['ANT']), int(d['FREQ']), int(d['TIME']))
        except (KeyError, ValueError):
            return 'MALFORMED', line
    head, sep, tail = line.partition(',')
    head = head.upper()
    if head in {'OK','ERROR'}:
        return head, tail
    if head in {'STATUS','VERSION','SERIAL','TEMP','REGION','ANTENNA','POWER','PROTOCOL','SESSION','FREQ','REGIONS'}:
        values={'_csv':[]}
        for p in [x.strip() for x in tail.split(',') if x.strip()]:
            if '=' in p:
                k,v=p.split('=',1); values[k.upper()] = v
            else: values['_csv'].append(p)
        return head, values
    return 'UNKNOWN', line
