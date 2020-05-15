#!/usr/bin/env python3

"""
setup.py file for SWIG example
"""

from distutils.core import setup, Extension

rawcam_module = Extension('_rawcam',
                          include_dirs=['/opt/vc/include'],
                          sources=['rawcam_wrap.c', 'rawcam.c'],
                          libraries=['mmal', 'bcm_host', 'vcos', 'mmal_core',
                                     'mmal_util', 'mmal_vc_client'],
                          library_dirs=['/opt/vc/lib'],
)

setup (name = 'rawcam',
       version = '0.1',
       author      = "H",
       description = """Raw camera interface""",
       ext_modules = [rawcam_module],
       py_modules = ["rawcam"],
       )
