#!/usr/bin/env python3
# rcon.py — drive the live ReHLDS test rig over UDP rcon (cont.93).
# The test server sets `rcon_password lol`. Usage:
#   python3 rcon.py status
#   python3 rcon.py "changelevel ts_hammertime"
#   python3 rcon.py "globalslow 50"
import socket, sys
HOST = ('127.0.0.1', 27015)
PW = 'lol'

def cmd(c, timeout=3):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.settimeout(timeout); s.connect(HOST)
    s.send(b'\xff\xff\xff\xffchallenge rcon\n')
    try:
        ch = s.recv(1024).split(b'challenge rcon ')[1].split(b'\n')[0].decode()
    except Exception as e:
        return f'(no challenge: {e})'
    s.send(b'\xff\xff\xff\xffrcon %s %s %s\n' % (ch.encode(), PW.encode(), c.encode()))
    out = b''
    try:
        while True:
            out += s.recv(4096)
    except socket.timeout:
        pass
    return out.decode('latin1', errors='replace')

if __name__ == '__main__':
    print(cmd(' '.join(sys.argv[1:])))
