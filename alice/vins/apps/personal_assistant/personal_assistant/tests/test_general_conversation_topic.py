# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from vins_core.dm.request import create_request
from personal_assistant.app import GeneralConversationTopics
import requests_mock
from requests.exceptions import ConnectTimeout

TEST_UUID = '00000000000000000000000000000001'
TEST_URL = 'http://alice.gc.dj.n.yandex-team.ru/recommender?experiment=topics&uuid=' + TEST_UUID
TEST_EXPERIMENTS = ('gc_skill_suggest_topic', 'personalized_topic')


def mock_topic(**kwargs):
    with requests_mock.Mocker() as m:
        m.get(TEST_URL, **kwargs)
        return GeneralConversationTopics().generate_topic(create_request(uuid=TEST_UUID, experiments=TEST_EXPERIMENTS), always_generate=True)


def test_general_conversation_topic():
    assert mock_topic(json={'items': [{'id': 'о фильмах'}]}) == 'о фильмах'


def test_general_conversation_topic_status_code_failure():
    assert mock_topic(json={'items': [{'id': 'о фильмах'}]}, status_code=404)


def test_general_conversation_topic_timeout_failure():
    assert mock_topic(exc=ConnectTimeout)


def test_general_conversation_topic_invalid_json_failure():
    assert mock_topic(text='invalid')


def test_general_conversation_topic_items_missing_failure():
    assert mock_topic(json={})


def test_general_conversation_topic_invalid_items_failure():
    assert mock_topic(json={'items': {}})


def test_general_conversation_topic_id_missing_failure():
    assert mock_topic(json={'items': [{}]})


def test_general_conversation_topic_invalid_id_failure():
    assert mock_topic(json={'items': [{'id': {}}]})
