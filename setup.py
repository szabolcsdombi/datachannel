import os
import sys
import glob

from setuptools import Extension, setup

libraries = ['datachannel']

deps = os.path.abspath(glob.glob('vcpkg_installed/x64-*/')[0])

if sys.platform.startswith('linux'):
    libraries.extend(['ssl', 'crypto', 'juice', 'usrsctp'])

ext = Extension(
    name='datachannel.datachannel',
    sources=['./datachannel.cpp'],
    define_macros=[
        ('Py_LIMITED_API', 0x03090000),
        ('PY_SSIZE_T_CLEAN', None),
    ],
    include_dirs=[f'{deps}/include'],
    library_dirs=[f'{deps}/lib'],
    libraries=libraries,
    py_limited_api=True,
)

setup(
    name='datachannel',
    version='0.4.0',
    packages=['datachannel'],
    ext_modules=[ext],
    # package_data={
    #     'datachannel': [
    #         'datachannel.dll',
    #         'juice.dll',
    #         'legacy.dll',
    #         'libcrypto-3-x64.dll',
    #         'libssl-3-x64.dll',
    #         'datachannel.pyd',
    #         'datachannel.abi3.so',
    #     ],
    # },
    # include_package_data=True,
    python_requires='>=3.9',
)
