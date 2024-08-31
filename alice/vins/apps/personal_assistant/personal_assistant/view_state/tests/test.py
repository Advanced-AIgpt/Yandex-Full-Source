# coding=utf-8

import codecs
import json
import os
import pytest


from alice.vins.apps.personal_assistant.personal_assistant.view_state.lib.extract_active_video_items import extract_active_video_items


ARCADIA_ROOT_PATH = os.environ['ARCADIA_ROOT']
VIEW_STATE_PATH = os.path.join(ARCADIA_ROOT_PATH, 'alice/vins/apps/personal_assistant/personal_assistant/view_state/tests/data/view_state.json')


@pytest.yield_fixture
def view_state():
    with codecs.open(VIEW_STATE_PATH, 'r', encoding='utf8') as f:
        yield json.load(f)


def test_get_active_video_items(view_state):
    video_numbers = map(lambda video_item: video_item['number'], extract_active_video_items(view_state))
    assert video_numbers == [17, 18, 19, 20]
