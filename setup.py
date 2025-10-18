#!/usr/bin/env python3
"""Setup script for meowline."""

from setuptools import setup

setup(
    name='meowline',
    version='0.1.0',
    description='A simple status line for your terminal',
    author='meowline contributors',
    py_modules=['meowline'],
    entry_points={
        'console_scripts': [
            'meowline=meowline:main',
        ],
    },
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Topic :: Terminals',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
)
