#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Yandex Uniproxy Streaming Client"""

from __future__ import absolute_import
import logging
import click
import sys
from connection import run
import uuid

try:
    import pyaudio
    is_pyaudio = True
except ImportError:
    is_pyaudio = False


DEFAULT_KEY_VALUE = "069b6659-984b-4c5f-880e-aaedcfd84102"
DEFAULT_KEY_VALUE = "developers-simple-key"
DEFAULT_URL_VALUE = "ws://localhost:8887/uni.ws"
DEFAULT_UUID_VALUE = str(uuid.uuid4())


@click.command()
@click.option('-k', '--key',
              help='You could get it at https://developer.tech.yandex.ru/. Default is "{0}".'.format(DEFAULT_KEY_VALUE),
              default=DEFAULT_KEY_VALUE)
@click.option('-u', '--url',
              help='Default is {0}.'.format(DEFAULT_URL_VALUE),
              default=DEFAULT_URL_VALUE)
@click.option('--uuid',
              default=DEFAULT_UUID_VALUE,
              help='UUID of your request. It can be helpful for further logs analysis. Default is generated uuid.')
@click.option('--vins',
              default=None,
              help='Vins request.')
def main(**kwars):
    logging.basicConfig(level=logging.INFO)

    if is_pyaudio:
        logging.getLogger("uniclient").info("Good for you")
    else:
        click.echo('Please install pyaudio module for system audio recording.')
        sys.exit(-2)

    run(**kwars)

if __name__ == "__main__":
        main()
