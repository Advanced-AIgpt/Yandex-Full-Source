# coding: utf-8

"""Colorized warning and error messages.
Uses devtools/ya/yalibrary/formatter/palette.py.
"""

from __future__ import print_function, unicode_literals

import sys


def error(message):
    print('[[bad]]Error:[[rst]] {}'.format(message), file=sys.stderr)


def warning(message):
    print('[[warn]]Warning:[[rst]] {}'.format(message), file=sys.stderr)
