# coding: utf-8
from __future__ import unicode_literals

import functools
import pytest
import yatest.common
from uuid import uuid4 as gen_uuid

import mock
import os

from vins_core.utils.data import find_vinsfile
from vins_core.dm.request import create_request
from vins_core.ext.general_conversation import GeneralConversationAPI, GCResponse
from vins_sdk.connectors import TestConnector

from personal_assistant.general_conversation import GeneralConversation

from gc_skill.app import ExternalSkillApp


@pytest.fixture(scope='session', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')


@pytest.fixture(scope='module')
def skills_vins_app():
    def get_s3_data(self, key, *args, **kwargs):
        return False, None, None

    with mock.patch('vins_core.ext.s3.S3DownloadAPI.get_if_modified', get_s3_data):
        with mock.patch.object(
            GeneralConversationAPI, 'handle',
            side_effect=lambda context, experiments: [
                GCResponse(text='Сам %s' % context[-1].lower(),
                           docid='123', relevance=1.0, source=None,
                           action=None)
            ]
        ), mock.patch.dict(os.environ, {'VINS_GC_MAX_SUGGESTS': '5'}):
            yield TestConnector(vins_app=ExternalSkillApp(seed=57, vins_file=find_vinsfile('gc_skill')))


@pytest.fixture
def f(skills_vins_app):
    return functools.partial(skills_vins_app.handle_utterance, str(gen_uuid()))


@pytest.fixture
def s(skills_vins_app):
    return functools.partial(skills_vins_app.handle_utterance, str(gen_uuid()), text_only=False)


def test_gc(f):
    assert f('ты китик') == 'Сам ты китик'


def test_gc_suggests(s):
    with mock.patch.object(
        GeneralConversationAPI, 'handle',
        side_effect=lambda context, experiments: [
            GCResponse(text='сам %s' % context[-1], docid='123', relevance=1.0, source=None, action=None)
        ]
    ):
        res = s('ты китик')
        assert res['voice_text'] == 'Сам ты китик'
        assert res['suggests'][0]['title'] == 'Сам сам ты китик'


def test_conversation_start(skills_vins_app, f):
    uuid = gen_uuid()
    req = create_request(uuid, 'ты китик', reset_session=True)
    resp = skills_vins_app.handle_request(req)
    text = '\n'.join(c.text for c in resp.cards)

    assert text == 'Давайте поболтаем, но учтите, что когда мы просто болтаем, я не пользуюсь Яндексом и могу отвечать странное. Чтобы узнавать погоду, курсы валют и искать в Яндексе, скажите "Хватит болтать" и потом задайте свой вопрос.'  # noqa
    assert f('ты китик') == 'Сам ты китик'


def test_error(f):
    with mock.patch.object(GeneralConversation, 'get_response_with_suggests', return_value=(None, [])):
        assert f('ты китик') == 'Что, простите?'

    assert f('ты китик') == 'Сам ты китик'


def test_alice_microintent(f):
    assert f('как тебя зовут') == 'Меня зовут Алиса.'


def test_skill_microintent(f):
    assert f('что ты умеешь') == 'Сейчас просто болтаем, а если нужно узнать что-то нужное — скажите "Хватит болтать" и задайте вопрос.'  # noqa


def test_ignore_lets_talk_microintent(f):
    assert f('давай поболтаем') == 'Сам давай поболтаем'


def test_microintent_sequences(f):
    f('как тебя зовут')
    f('ты когда родилась')
    assert f('а в каком году') == 'Отлично, вы меня ещё про возраст спросите.'
    assert f('а в каком году') == 'Сам а в каком году'


def test_short_welcome(skills_vins_app, f):
    uuid = gen_uuid()

    def start_session():
        req = create_request(uuid, 'ты китик', reset_session=True)
        resp = skills_vins_app.handle_request(req)
        return '\n'.join(c.text for c in resp.cards)

    assert start_session() == 'Давайте поболтаем, но учтите, что когда мы просто болтаем, я не пользуюсь Яндексом и могу отвечать странное. Чтобы узнавать погоду, курсы валют и искать в Яндексе, скажите "Хватит болтать" и потом задайте свой вопрос.'  # noqa
    assert start_session() == 'Начинайте.'


def test_suggest_topic(skills_vins_app, f):
    uuid = gen_uuid()

    def start_session():
        req = create_request(uuid, 'ты китик', reset_session=True, experiments=('gc_skill_suggest_topic',))
        resp = skills_vins_app.handle_request(req)
        return '\n'.join(c.text for c in resp.cards)

    assert start_session() == 'Давайте поболтаем, но учтите, что когда мы просто болтаем, я не пользуюсь Яндексом и могу отвечать странное. Чтобы узнавать погоду, курсы валют и искать в Яндексе, скажите "Хватит болтать" и потом задайте свой вопрос. Давайте поговорим о сказках.'  # noqa
    assert start_session() == 'Давайте поговорим о песнях.'
