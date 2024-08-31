# coding: utf-8
from __future__ import unicode_literals

import os
import pytest
import yatest.common

from vins_tools.nlu.inspection.interactive_app_analyzer import InteractiveAppAnalyzer


@pytest.fixture(scope='session', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')


def test_analyzer_loads():
    analyzer = InteractiveAppAnalyzer('personal_assistant')
    hypotheses = analyzer.get_hypotheses('включи радио рама', 15)
    assert len(hypotheses) == 15
    assert hypotheses.min() >= -1
    assert hypotheses.max() <= 1
    assert 'personal_assistant.scenarios.music_play' in hypotheses.index
    assert 'personal_assistant.scenarios.radio_play' in hypotheses.index
