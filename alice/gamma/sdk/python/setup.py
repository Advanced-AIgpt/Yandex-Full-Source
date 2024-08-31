#!/usr/bin/env python

from setuptools import setup

setup(name='gamma sdk',
      version='0.1',
      description='Gamma skill engine sdk',
      author='Grigory Kostin',
      author_email='g-kostin@yandex-team.ru',
      packages=['gamma_sdk', 'gamma_sdk.inner'],
      install_requires=[
            'enum34>=1.1',
            'attrs==18.2.0',
            'grpcio==1.18',
            'grpcio-tools==1.18'
      ],
      )
