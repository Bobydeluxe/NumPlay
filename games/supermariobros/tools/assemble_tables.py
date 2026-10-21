from pathlib import Path
import re,sys,json,hashlib
from py65.assembler import Assembler
from py65.devices.mpu6502 import MPU

root=Path(__file__).resolve().parents[1]/'assets'
if len(sys.argv)!=2:raise SystemExit('Usage: python tools/assemble_tables.py path/to/smbdis.asm')
source=Path(sys.argv[1]).read_text()
symbols={}
aliases={}
lines=[]
for text in source.splitlines():
    text=text.split(';',1)[0].strip()
    if not text:continue
    m=re.match(r'(\w+)\s*=\s*(.*)',text)
    if m:
        value=m[2].replace('$','0x').replace('%','0b')
        flag=value[0] if value[0] in '<>' else ''
        try:v=int(value[1:] if flag else value,0)
        except ValueError:
            aliases[m[1]]=m[2]
            continue
        symbols[m[1]]=v&255 if flag=='<' else v>>8 if flag=='>' else v
        continue
    label=None
    if ':' in text:label,text=text.split(':',1);text=text.strip()
    if text.startswith(('.index','.mem')):continue
    lines.append((label,text))

def expr(text,default=0x8000):
    text=text.strip()
    if text.startswith('<'):return expr(text[1:],default)&255
    if text.startswith('>'):return expr(text[1:],default)>>8
    text=text.replace('$','0x').replace('%','0b')
    text=re.sub(r'\b[A-Za-z_]\w*\b',lambda m:str(symbols.get(m[0],default)) if not m[0].startswith(('x','b')) else m[0],text)
    return eval(text,{'__builtins__':{}},{})

asm=Assembler(MPU())
def emit(text,pc):
    if not text:return []
    if text.startswith('.org'):return []
    if text.startswith(('.db','.dw')):
        words=text.startswith('.dw');out=[]
        for item in text[3:].split(','):
            v=expr(item);out.append(v&255)
            if words:out.append(v>>8)
        return out
    op,*args=text.split(None,1)
    arg=args[0].strip() if args else ''
    if not arg or arg.lower()=='a':return asm.assemble(op.upper()+(' A' if arg else ''),pc)
    m=re.fullmatch(r'([#(]?)([^,)]+)(\)?(?:,[xyXY])?\)?)',arg)
    assert m,(text,arg)
    prefix,val,suffix=m.groups();v=expr(val)
    if prefix=='#':arg=prefix+'$'+format(v&255,'02X')+suffix
    else:arg=prefix+'$'+format(v,'02X' if v<256 else '04X')+suffix
    return asm.assemble(op.upper()+' '+arg.upper(),pc)

for passnum in range(5):
    for key,value in aliases.items():symbols[key]=expr(value)
    pc=0x8000
    for label,text in lines:
        if label:symbols[label]=pc
        if text.startswith('.org'):pc=expr(text[4:]);continue
        try:pc+=len(emit(text,pc))
        except Exception as e:raise RuntimeError((hex(pc),label,text,str(e)))
assert pc==0x10000,hex(pc)
data=bytearray(0x8000)
data_ranges=[]
pc=0x8000
for label,text in lines:
    if text.startswith('.org'):pc=expr(text[4:]);continue
    chunk=emit(text,pc);data[pc-0x8000:pc-0x8000+len(chunk)]=bytes(chunk)
    if text.startswith(('.db','.dw')) and chunk:
        if data_ranges and data_ranges[-1][1]==pc:data_ranges[-1][1]+=len(chunk)
        else:data_ranges.append([pc,pc+len(chunk)])
    pc+=len(chunk)
assert hashlib.sha256(data).hexdigest()=='5374abb64cfb9b5d961856c60166cedc55859164cf2b8867730e144fa2bdd594'
(root/'tables.bin').write_bytes(data)
(root/'symbols.json').write_text(json.dumps(symbols,indent=2))
(root/'data-ranges.json').write_bytes((json.dumps(data_ranges,indent=2)+'\n').encode())
print('PRG',len(data),hashlib.sha256(data).hexdigest(),hex(symbols['WorldAddrOffsets']))
