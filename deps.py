import subprocess
import glob
import shutil
import os
import sys

subprocess.call(['vcpkg', 'install'])

if sys.platform.startswith('win'):
    for f in glob.glob('vcpkg_installed/x64-windows/bin/*.dll'):
        shutil.copy(f, os.path.basename(f))
