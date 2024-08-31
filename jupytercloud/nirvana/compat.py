# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path

try:
    from functools import lru_cache
except ImportError:
    from backports.functools_lru_cache import lru_cache

try:
    from functools import cached_property
except ImportError:
    from cached_property import cached_property


__all__ = [
    'Path',
    'lru_cache',
    'cached_property',
]
