# coding: utf-8
from __future__ import unicode_literals


def relative_datetime_raw(dt):
    for k, v in dt.items():
        if k.endswith('_relative') and v:
            return True
    return False
