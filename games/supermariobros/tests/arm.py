#!/usr/bin/env python3
"""Execute the compiled app on all four NumPlay ARM emulator profiles.
Build first and run tests/test.c to produce save fixtures. Requires Unicorn,
Pillow and pyelftools. --nwlink accepts a command wrapper on Windows.
"""
import argparse
import io
import json
from pathlib import Path
import struct
import tempfile
import types

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output'
emu_path=ROOT.parents[1]/'tools/emu.py'
emu=types.ModuleType('mario_emu');emu.__file__=str(emu_path);emu.io=io
source=emu_path.read_text().replace('ELFFile(open(elf, "rb"))','ELFFile(io.BytesIO(open(elf, "rb").read()))')
source=source.replace('chunk = CPU_HZ // 1000','chunk = CPU_HZ // 10000')
source=source.replace('self.now_ms += 1\n','self.now_ms += 0.1\n')
# A static menu can execute many sleeps in a single instruction slice. Stop
# at a completed sleep when a checkpoint is reached; CPU/game state is intact.
source=source.replace('        next_frame = self.now_ms if frame_ms else None',
    '        self.checkpoint = until_ms\n        next_frame = self.now_ms if frame_ms else None')
source=source.replace('            self.slept_ms += r0\n',
    '            self.slept_ms += r0\n            if self.now_ms >= self.checkpoint: uc.emu_stop()\n')
exec(compile(source,str(emu_path),'exec'),emu.__dict__)

def record(name,value):
    data=name.encode()+b'\0'+value
    return struct.pack('<H',len(data)+2)+data

def records(data):
    result={};pos=0
    while True:
        n,=struct.unpack_from('<H',data,pos)
        if not n:return result
        assert n>=4 and pos+n<=len(data)
        end=data.index(0,pos+2,pos+n)
        result[data[pos+2:end].decode()]=data[end+1:pos+n];pos+=n

def presses(*items):return [(t,t+d,emu.KEYS[k]) for t,k,d in items]
def state(c):return bytes(c.uc.mem_read(c.symbols['game'],2048))
def u32(c,s):return struct.unpack('<I',c.uc.mem_read(c.symbols[s],4))[0]
def safe(c,exited=True):
    assert c.exited==exited
    assert not c.violations,c.violations
    assert not c.firmware_ram_changes(),c.firmware_ram_changes()
    if exited:assert c.app_ram_clean() and c.locks==0
def performance(c):
    result={k:u32(c,k) for k in ('perf_frames','perf_max','perf_slow')}
    assert result['perf_max']<=33 and result['perf_slow']==0,result
    return result

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--nwlink',default='nwlink');a=ap.parse_args()
    (OUT/'tmp').mkdir(exist_ok=True);tempfile.tempdir=str(OUT/'tmp');report=[]
    nwa=str(OUT/'supermariobros-debug.nwa')
    profiles=[('n0110','n0110','epsilon',148928),('n0115-custom','n0110','epsilon',133652),('n0120','n0120','epsilon',153676),('upsilon','n0110','upsilon',107674)]
    for name,model,system,ram in profiles:
        emu.configure(model,system,ram)
        storage=OUT/(name+'-storage.bin');storage.write_bytes(record('keep.py',b'print(42)\n')+b'\0\0')
        c=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c.enable_checks()
        c.keys=presses((0,'exe',300),(650,'exe',70),(4200,'right',500),(4300,'ok',170),(4900,'back',70),(5500,'down',70),(5700,'ok',70),(5900,'back',70),(6100,'up',70),(6300,'ok',70),(6800,'home',100))
        c.run(500);assert state(c)[0x770]==0,'Held EXE started the game';c.save_png(str(OUT/(name+'-title.png')))
        c.run(4000);assert state(c)[0x770]==1;c.save_png(str(OUT/(name+'-gameplay.png')))
        c.run(5250);frozen=state(c);saved=records(c.storage_bytes())['supermario.sav'];assert len(saved)==4404 and saved[:4]==b'SMN1'
        c.save_png(str(OUT/(name+'-pause.png')))
        c.run(6050);assert state(c)==frozen,'Pause/restart cancellation advanced the game'
        perf=performance(c);c.run(7250);safe(c);stack=c.stack_used()
        recs=records(c.storage_bytes());assert recs['keep.py']==b'print(42)\n';saved=recs['supermario.sav']
        c2=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c2.enable_checks()
        c2.keys=presses((500,'ok',70),(950,'right',120),(1300,'onoff',100))
        c2.run(350);assert state(c2)==saved[16:2064],'Save not restored exactly';c2.save_png(str(OUT/(name+'-restored.png')))
        c2.run(1700);safe(c2);assert records(c2.storage_bytes())['keep.py']==b'print(42)\n'
        # Explicit fault injection verifies the embedded recovery context.
        storage=OUT/(name+'-fault-storage.bin');raw=(OUT/'start-save.bin').read_bytes()
        storage.write_bytes(record('supermario.sav',raw)+b'\0\0')
        c3=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c3.enable_checks()
        c3.keys=presses((500,'ok',70),(1300,'home',100));c3.run(250)
        c3.uc.mem_write(c3.symbols['game']+0x772,b'\xff')
        c3.run(1000);assert state(c3)[0x772]==255;c3.save_png(str(OUT/(name+'-fault.png')))
        c3.run(1700);safe(c3);assert records(c3.storage_bytes())['supermario.sav']==raw
        # Final-world state machine and B-button second-quest unlock.
        storage=OUT/(name+'-ending-storage.bin')
        storage.write_bytes(record('supermario.sav',(OUT/'ending-save.bin').read_bytes())+b'\0\0')
        c4=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c4.enable_checks()
        c4.keys=presses((500,'ok',70),(800,'shift',100),(1500,'home',100))
        c4.run(1250);assert state(c4)[0x7fc];c4.run(1900);safe(c4)
        report.append(dict(profile=name,stack=max(stack,c2.stack_used()),step_max=u32(c,'step_max'),**perf));print(json.dumps(report[-1]),flush=True)
    # The actual ARM program loads and plays every original level, including
    # automatic entrance-pipe transitions already exercised by the host suite.
    emu.configure('n0110','epsilon',148928);levels=[]
    for w in range(1,9):
      for l in range(1,5):
        storage=OUT/'level-storage.bin';raw=(OUT/f'level-{w}-{l}.bin').read_bytes()
        storage.write_bytes(record('keep.py',b'level sentinel\n')+record('supermario.sav',raw)+b'\0\0')
        c=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c.enable_checks()
        c.keys=presses((350,'ok',70),(550,'right',140),(550,'shift',140),(600,'ok',80),(1200,'home',100))
        c.run(250);assert state(c)==raw[16:2064]
        c.run(950);g=state(c);assert g[0x75f]==w-1 and g[0x75c]==l-1,(w,l,g[0x75f],g[0x75c])
        c.save_png(str(OUT/f'arm-level-{w}-{l}.png'));perf=performance(c)
        c.run(1600);safe(c);assert records(c.storage_bytes())['keep.py']==b'level sentinel\n'
        levels.append(dict(world=w,level=l,stack=c.stack_used(),**perf))
      print(f'ARM world {w}: all four levels pass',flush=True)
    # Both original players share the calculator, taking turns after a death.
    storage=OUT/'players-storage.bin';storage.write_bytes(b'\0\0')
    c=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c.enable_checks()
    c.keys=presses((500,'alpha',70),(850,'exe',70),(4200,'toolbox',70),(4800,'back',70),(5200,'home',100))
    c.run(4000);assert state(c)[0x77a]==1,'Two-player selection failed'
    c.run(4600);c.save_png(str(OUT/'arm-guide.png'));c.run(5600);safe(c)
    # No-space behavior must remain interactive and preserve the full record.
    storage=OUT/'full-storage.bin';full=record('large.py',b'#'*(emu.STORAGE_SIZE-16));storage.write_bytes(full+b'\0\0')
    c=emu.Calculator(nwa,storage_file=str(storage),nwlink=a.nwlink);c.enable_checks()
    c.keys=presses((500,'home',300),(1300,'1',70),(1650,'home',100),(2000,'ok',70),(2400,'back',70))
    c.run(1000);safe(c,False);c.save_png(str(OUT/'arm-full-storage.png'));assert c.storage_bytes()[:len(full)]==full
    c.run(2800);safe(c);assert c.storage_bytes()[:len(full)]==full
    result=dict(profiles=report,levels=levels)
    (OUT/'arm-validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: four profiles, all 32 ARM level scenarios, pause/resume, both players, full storage and safe exits',flush=True)
if __name__=='__main__':main()
