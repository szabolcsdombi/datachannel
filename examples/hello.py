import time

import datachannel

p1 = datachannel.peer()
p2 = datachannel.peer(p1.sdp())
p1.connect(p2.sdp())

p1.wait()
p2.wait()

p1.send(b'hello world')
time.sleep(0.5)
print(p2.recv())
