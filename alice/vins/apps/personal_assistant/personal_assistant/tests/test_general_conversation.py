# coding: utf-8
from __future__ import unicode_literals

from vins_core.common.sample import Sample
from vins_core.dm.response import VinsResponse
from vins_core.dm.request import AppInfo, create_request
from vins_core.dm.session import Session
from vins_core.common.utterance import Utterance
from vins_core.ext.general_conversation import GCResponse

from personal_assistant.general_conversation import GeneralConversation


def test_prev_phrases():
    session = Session('test-app', 'test-uuid')
    dialog_history = session.dialog_history

    gc = GeneralConversation()
    assert gc.get_context(session, Sample.from_string('ok')) == ['ok']

    dialog_history.add(
        utterance=Utterance('привет'),
        response=VinsResponse().say('здравствуй'),
    )
    assert gc.get_context(session, Sample.from_string('ok')) == [
        'привет', 'здравствуй', 'ok',
    ]

    dialog_history.add(
        utterance=Utterance('расскажи про новости'),
        response=VinsResponse(),
    )
    dialog_history.add(
        utterance=None,
        response=VinsResponse().say('всё как обычно'),
    )
    assert gc.get_context(session, Sample.from_string('ok')) == [
        'расскажи про новости', 'всё как обычно', 'ok',
    ]

    dialog_history.add(
        utterance=Utterance('про погоду говорю расскажи'),
        response=VinsResponse(),
    )
    dialog_history.add(
        utterance=None,
        response=VinsResponse().say('что-то пошло не так'),
    )
    dialog_history.add(
        utterance=None,
        response=VinsResponse().say('всё как обычно'),
    )
    assert gc.get_context(session, Sample.from_string('ok')) == [
        'про погоду говорю расскажи', 'что-то пошло не так. всё как обычно', 'ok',
    ]


def test_select_gc_phrase_returns_question_for_experiment():
    gc = GeneralConversation(gc_force_question_top_k=4)
    context = ['привет']
    phrases = ['привет!', 'как дела?', 'ты китик', 'халло', 'как тебя зовут?']
    responses = [GCResponse(text=p, docid=None, relevance=None, source=None, action=None) for p in phrases]
    req_info = create_request('123', experiments=['gc_force_question'])
    gc_response = gc._select_gc_response(responses, context, req_info, [])
    gc_phrase = gc_response.text if gc_response is not None else None
    assert gc_phrase == 'как дела?'


def test_select_gc_phrase_returns_random_without_questions():
    gc = GeneralConversation(gc_force_question_top_k=4)
    context = ['привет']
    phrases = ['привет!', 'как дела', 'ты китик']
    responses = [GCResponse(text=p, docid=None, relevance=None, source=None, action=None) for p in phrases]

    req_info = create_request('123', experiments=['gc_force_question'])
    sampled_answers = set()
    for i in range(10000):
        gc_response = gc._select_gc_response(responses, context, req_info, [])
        gc_phrase = gc_response.text if gc_response is not None else None
        sampled_answers.add(gc_phrase)

    assert len(sampled_answers) == len(phrases)

    req_info = create_request('123')
    sampled_answers = set()
    for i in range(10000):
        gc_response = gc._select_gc_response(responses, context, req_info, [])
        gc_phrase = gc_response.text if gc_response is not None else None
        sampled_answers.add(gc_phrase)

    assert len(sampled_answers) == len(phrases)


def test_no_repeat_gc():
    gc = GeneralConversation()
    context = ['0']
    phrases = [str(i) for i in range(15)]
    responses = [GCResponse(text=p, docid=None, relevance=None, source=None, action=None) for p in phrases]
    req_info = create_request('123')
    sampled_answers = set()
    used_replies = []
    for i in range(15):
        gc_response = gc._select_gc_response(responses, context, req_info, used_replies)
        gc_phrase = gc_response.text if gc_response is not None else None
        sampled_answers.add(gc_phrase)
    assert len(sampled_answers) == len(phrases)


def test_gc_source():
    gc = GeneralConversation()
    context = ['привет']
    response = GCResponse(text='привет!', docid=None, relevance=None, source='test_source', action=None)
    req_info = create_request('123')
    gc_response = gc._select_gc_response([response], context, req_info, [])
    phrase = gc_response.text if gc_response is not None else None
    assert phrase == 'привет!'
    source = gc_response.source if gc_response is not None else None
    assert source == 'test_source'


def test_proactivity_music_for_quasar():
    gc = GeneralConversation()
    app_info = AppInfo(app_id='ru.yandex.quasar')
    for is_tv_plugged_in in [True, False]:
        device_state = {"is_tv_plugged_in": is_tv_plugged_in}
        req_info = create_request(
            uuid='234',
            utterance='мне нечем заняться',
            app_info=app_info,
            device_state=device_state)
        context = ['мне нечем заняться']
        response = GCResponse(text='Хотите послушать музыку?', docid=None, relevance=None, source='proactivity', action='personal_assistant.scenarios.music_play')

        gc_response = gc._select_gc_response([response], context, req_info, [])

        assert gc_response is not None
        assert gc_response.source == 'proactivity'
        assert gc_response.text == 'Хотите послушать музыку?'
        assert gc_response.action == 'personal_assistant.scenarios.music_play'


def test_proactivity_video_for_quasar():
    gc = GeneralConversation()
    app_info = AppInfo(app_id='ru.yandex.quasar')
    for is_tv_plugged_in in [True, False]:
        device_state = {"is_tv_plugged_in": is_tv_plugged_in}
        req_info = create_request(
            uuid='234',
            utterance='мне нечем заняться',
            app_info=app_info,
            device_state=device_state)
        context = ['мне нечем заняться']
        response = GCResponse(text='Хотите посмотреть фильм?', docid=None, relevance=None, source='proactivity', action='personal_assistant.scenarios.video_general_scenario')

        gc_response = gc._select_gc_response([response], context, req_info, [])

        if is_tv_plugged_in:
            assert gc_response is not None
            assert gc_response.source == 'proactivity'
            assert gc_response.text == 'Хотите посмотреть фильм?'
            assert gc_response.action == 'personal_assistant.scenarios.video_general_scenario'
        else:
            assert gc_response is None


def test_proactivity_for_quasar():
    gc = GeneralConversation()
    app_info = AppInfo(app_id='ru.yandex.quasar')
    req_info = create_request(uuid='234', app_info=app_info, experiments={'gc_proactivity'})

    experiments = gc._get_patched_experiments(req_info, is_pure_gc=False, is_suggests=False)
    assert experiments['gc_proactivity'] is not None

    for is_pure_gc, is_suggests in [(False, True), (True, False), (True, True)]:
        experiments = gc._get_patched_experiments(req_info, is_pure_gc=is_pure_gc, is_suggests=is_suggests)
        assert experiments['gc_proactivity'] is None


def test_proactivity_for_not_quasar():
    gc = GeneralConversation()
    req_info = create_request(uuid='234', experiments={'gc_proactivity'})

    for is_pure_gc, is_suggests in [(False, True), (True, False), (True, True), (False, False)]:
        experiments = gc._get_patched_experiments(req_info, is_pure_gc=is_pure_gc, is_suggests=is_suggests)
        assert experiments['gc_proactivity'] is None
