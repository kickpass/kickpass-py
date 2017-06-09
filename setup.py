#!/usr/bin/env python3
from setuptools import setup, Extension

setup(name='kickpass',
      version='0.2.0',
      ext_modules=[Extension('kickpass', ['kickpass.c'], libraries=['kickpass'],
                             extra_compile_args=['-DLIBBSD_OVERLAY', '-D_GNU_SOURCE',
                                                 '-O0', '-g'])],
      test_suite="tests")
