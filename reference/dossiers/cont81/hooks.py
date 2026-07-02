import struct
DLL='/home/stevenlafl/Projects/goldsrc.net/mod-ts/reference/client.dll'
data=open(DLL,'rb').read()
pe=struct.unpack_from('<I',data,0x3c)[0]
nsec=struct.unpack_from('<H',data,pe+6)[0]
opt=struct.unpack_from('<H',data,pe+20)[0]
imagebase=struct.unpack_from('<I',data,pe+24+28)[0]
secs=[]
o=pe+24+opt
for _ in range(nsec):
    vsize,va,rsize,ro=struct.unpack_from('<IIII',data,o+8)
    secs.append((va,vsize,ro,rsize)); o+=40
def va2off(va):
    for v,vs,ro,rs in secs:
        if v<=va<v+max(vs,rs): return ro+(va-v)
    return None
def off2va(off):
    for v,vs,ro,rs in secs:
        if ro<=off<ro+rs: return v+(off-ro)
    return None
def rdstr(va):
    o=va2off(va)
    if o is None: return None
    e=data.find(b'\0',o,o+64)
    if e<0: return None
    s=data[o:e]
    try:
        d=s.decode()
        return d if d and all(32<=ord(c)<127 for c in d) else None
    except: return None
sig=b'\xff\x15'+struct.pack('<I',imagebase+0xd5b70)
print('sig hits:',data.count(sig))
i=0; hooks={}
while True:
    i=data.find(sig,i)
    if i<0: break
    win=data[i-20:i]
    k=0; ps=[]
    while k<len(win):
        if win[k]==0x68 and k+5<=len(win):
            ps.append(struct.unpack_from('<I',win,k+1)[0]); k+=5
        else: k+=1
    if len(ps)>=2:
        a,b=ps[-2],ps[-1]
        nm=rdstr(b); h=a
        if nm is None:
            nm=rdstr(a); h=b
        if nm:
            hooks[nm]=h
        else:
            print('unresolved at file off',hex(i),['%08x'%p for p in ps])
    else:
        print('few pushes at',hex(i),['%08x'%p for p in ps])
    i+=1
print(len(hooks),'hooks')
for k in sorted(hooks): print('%-14s 0x%08x'%(k,hooks[k]))
