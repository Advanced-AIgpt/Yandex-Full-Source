#!/usr/bin/env python
# encoding: utf-8

import json
from library.python import resource


def test_load_json():
    config = json.loads(resource.find('toloka_projects_config.json'))
    assert True, 'json корректно загрузился'
    assert 0 <= config['screenshots_music_percent'] <= 100, 'screenshots_music_percent должен быть в диапазоне [0; 100]'
    assert 0 <= config['screenshots_facts_percent'] <= 100, 'screenshots_facts_percent должен быть в диапазоне [0; 100]'
