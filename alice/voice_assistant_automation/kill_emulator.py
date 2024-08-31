import os
import subprocess
import sys
import time

for p in subprocess.check_output("ps aux | grep emulator", shell=True).split("\n"):
	if sys.argv[0] in p:
        continue
    try:
        os.kill(int(p.split()[1]), 15)
    except:
        pass
time.sleep(2)
