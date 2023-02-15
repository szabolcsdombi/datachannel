import base64
import time

import datachannel
import pyperclip

p = datachannel.peer()

pyperclip.copy(base64.b64encode(p.sdp()).decode())

p.connect(base64.b64decode(input()))
print('waiting')
p.wait()
print('wait done')

while True:
    while msg := p.recv():
        print(f'received {msg}')
        p.send(msg)
    time.sleep(0.1)
