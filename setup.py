#!/usr/bin/env python3
from setuptools import setup, Extension

setup(name='kickpass',
      version='0.2.0',
      ext_modules=[Extension('kickpass', ['kickpass.c'], libraries=['kickpass'])])
