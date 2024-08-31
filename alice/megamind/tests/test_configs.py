# -*- coding: utf-8 -*-

import os
import subprocess

import pytest
import yatest

_VALIDATOR = os.path.join(yatest.common.build_path('alice/megamind/tools/config_validator'), 'config_validator')


def get_config(name=None):
    if name:
        return yatest.common.source_path(os.path.join('alice/megamind/configs', name, 'megamind.pb.txt'))
    return None


@pytest.mark.parametrize('config_name', [
    'dev',
    'production',
    'hamster'
])
def test_validate_config(config_name):
    current_config = get_config(config_name)
    cmd = [_VALIDATOR, '-c', current_config, '-p', '1']
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        pytest.fail(stderr)
