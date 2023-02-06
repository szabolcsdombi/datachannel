import os
import sys

from setuptools import Extension, setup

DEPENDENCIES = os.environ['DEPENDENCIES']
# C:/vcpkg/installed/x64-windows
# /opt/vcpkg/installed/x64-linux

libraries = ['datachannel']

if sys.platform.startswith('linux'):
    libraries.extend(['ssl', 'crypto', 'juice', 'usrsctp'])

ext = Extension(
    name='datachannel',
    sources=['./datachannel.cpp'],
    define_macros=[
        ('Py_LIMITED_API', 0x03090000),
        ('PY_SSIZE_T_CLEAN', None),
    ],
    include_dirs=[f'{DEPENDENCIES}/include'],
    library_dirs=[f'{DEPENDENCIES}/lib'],
    libraries=libraries,
    py_limited_api=True,
)

setup(
    name='datachannel',
    version='0.2.0',
    ext_modules=[ext],
    data_files=[('.', ['juice.dll', 'libcrypto-3-x64.dll', 'libssl-3-x64.dll'])],
)
