"""USB diagnostics for CPC-ADV; uses only Python's standard library."""
import os
import select
import struct
import sys
import termios
import time

port, command = sys.argv[1:3]
fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
settings = termios.tcgetattr(fd)
settings[0] = settings[1] = settings[3] = 0
settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
settings[4] = settings[5] = termios.B115200
settings[6][termios.VMIN] = 0
settings[6][termios.VTIME] = 0
termios.tcsetattr(fd, termios.TCSANOW, settings)
termios.tcflush(fd, termios.TCIFLUSH)
os.write(fd, command.encode('ascii'))
data = bytearray()
capture = command in ('s', 'v')
width, height = (240,135) if command == 'v' else (320,200)
header = ('CPCFRAME %d %d\n' % (width,height)).encode()
header565 = ('CPCFRAME565 %d %d\n' % (width,height)).encode()
count = width*height
deadline = time.monotonic() + (15 if capture else 3)
while time.monotonic() < deadline:
    if select.select([fd], [], [], 0.1)[0]:
        chunk = os.read(fd, 65536)
        data.extend(chunk)
        if capture and (header in data or header565 in data):
            rgb565=header565 in data
            marker=header565 if rgb565 else header
            at = data.index(marker) + len(marker)
            size=count*(2 if rgb565 else 1)
            if len(data) >= at + size:
                with open(sys.argv[3], 'wb') as f:
                    f.write(('P6\n%d %d\n255\n' % (width,height)).encode())
                    if rgb565:
                        for (pixel,) in struct.iter_unpack('<H',data[at:at+size]):
                            f.write(bytes(((pixel>>11)*255//31,((pixel>>5)&63)*255//63,(pixel&31)*255//31)))
                    else:
                        for pixel in data[at:at+count]:
                            levels = (0,128,128,255)
                            f.write(bytes((levels[pixel & 3], levels[(pixel>>2)&3], levels[(pixel>>4)&3])))
                print('Captured:', sys.argv[3])
                break
else:
    if capture:
        raise SystemExit('No complete framebuffer received: %d bytes' % len(data))
os.close(fd)
if not capture:
    print(data.decode('utf-8', errors='replace'))
