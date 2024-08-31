# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

from .operation import (
    is_nirvana,
    set_expected_inputs,
    get_inputs,
    get_named_inputs,
    get_custom_outputs,
    get_input,
    get_named_input,
    get_custom_output
)

from .yav import (
    get_secret,
    get_secret_key,
)

__all__ = [
    'is_nirvana',
    'set_expected_inputs',
    'get_inputs',
    'get_named_inputs',
    'get_custom_outputs',
    'get_input',
    'get_named_input',
    'get_custom_output',
    'get_secret',
    'get_secret_key',
]
