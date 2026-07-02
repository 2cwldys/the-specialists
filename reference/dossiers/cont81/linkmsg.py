import re, subprocess
SO='/home/stevenlafl/Projects/goldsrc.net/mod-ts/reference/ts_i386.so'
GOT=0x165e10
data=open(SO,'rb').read()
# gmsg symbol addresses
nm=subprocess.run(['nm',SO],capture_output=True,text=True).stdout
sym={}
for l in nm.splitlines():
    p=l.split()
    if len(p)==3 and p[2].startswith('gmsg'):
        sym[int(p[0],16)]=p[2]
# GOT relocations: slot -> symbol value it points to
# GLOB_DAT relocs
rel=subprocess.run(['readelf','-W','-r',SO],capture_output=True,text=True).stdout
gotmap={}
for l in rel.splitlines():
    m=re.match(r'^([0-9a-f]{8})\s+\S+\s+R_386_GLOB_DAT\s+([0-9a-f]{8})\s+(\S+)',l)
    if m:
        gotmap[int(m.group(1),16)]=(int(m.group(2),16),m.group(3))
dis=subprocess.run(['objdump','-d','--start-address=0xc7db0','--stop-address=0xc8460',SO],capture_output=True,text=True).stdout
size=None; straddr=None
out=[]
for l in dis.splitlines():
    m=re.search(r'push\s+\$0x([0-9a-f]+)',l)
    if m:
        v=int(m.group(1),16)
        if v==0xffffffff: v=-1
        size=v; continue
    m=re.search(r'lea\s+(-?0x[0-9a-f]+)\(%ebx\),%eax',l)
    if m:
        straddr=GOT+int(m.group(1),16); continue
    m=re.search(r'mov\s+0x([0-9a-f]+)\(%ebx\),%(edx|edi)',l)
    if m and straddr is not None:
        slot=GOT+int(m.group(1),16)
        name=data[straddr:data.index(b'\0',straddr)].decode()
        g=gotmap.get(slot,(0,'?'))
        out.append((name,size,g[1],hex(g[0])))
        size=None; straddr=None
for o in out: print('%-14s size=%-3s -> %s @%s'%o)
print(len(out),'registrations')
