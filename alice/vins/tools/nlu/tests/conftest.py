# coding: utf-8
from __future__ import unicode_literals

import pytest


@pytest.fixture(scope='function')
def test_app():
    return 'vins_core.test.test_data.app::TestApp'
