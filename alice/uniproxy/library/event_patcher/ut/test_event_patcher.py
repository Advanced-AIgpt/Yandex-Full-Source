import pytest

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.event_patcher import EventPatcher, MultiPatcher
from alice.uniproxy.library.logging import Logger


def test_event_patcher():
    Logger.init("unittest", True)

    event_patcher = EventPatcher([
        ['set', '.a.b.c1', 1],
        ['set', '/a/b/c2', 1],
        ['set', '@a@b@c3', 1],
        ['set', ['a', 'b', 'c4'], 1],
        ['set', '.a.b.c5', 1, 'set', '.a.b.c6', [], 'set', '.a.b.c7', '1'],
        ['set', '.set./set.@set', 1],

        ['append', '.experiments1', 'b'],
        ['append', '.experiments2', 'c'],
        ['append', '.experiments3', 'd'],
        ['append', '.experiments4', 'd'],
        ['append', '.experiments.experiments5', 'e'],
        ['extend', '.experiments.experiments6', ['e', 't']],
        ['extend', '.experiments7', ['f', 'g']],
        ['extend', '.experiments8', ['a', 's']],

        ['set_if_none', '.set_if_none_case.a', 'fail'],
        ['set_if_none', '.set_if_none_case.c', 'ok'],

        ['del', '.D.E.L1'],
        ['del', '/D/E/L2'],
        ['del', '@D@E@L3'],
        ['del', ['D', 'E', 'L4']],
        ['del', '.del.del/.del@'],

        ['if_event_type', 'asR.recognizE', 'set', '/asr/f1/f2', 'some_value'],
        ['if_event_type', 'OOO.recognizE', 'set', '/asr/f1/f3', 'some_value'],

        ['if_has_payload', '/voice/gender', 'set', '/voice/gender', 'female'],
        ['if_has_payload', '/voice/volume', 'set', '/voice/volume', 10],

        ['if_payload_eq', '/val', 1, 'set', '/cond1', 'ok'],
        ['if_payload_eq', '/value/x/y', 'xxx', 'set', '/cond2', 'fail'],

        ['if_payload_like', '/value/x/w', 'x.*x', 'set', '/cond3', 'ok'],
        ['if_payload_like', '/value/x/z', 'x.*x', 'set', '/cond4', 'fail'],

        ['if_event_type', 'ASR.Recognize', 'if_payload_eq', '/chain/val1', 42, 'if_payload_eq', '/chain/val2', 'str', 'set', '/chain/newval', 'ok'],

        ['if_has_session_data', '/sd/gender', 'set', '/sd/voice/gender', 'female'],
        ['if_has_session_data', '/sd/volume', 'set', '/sd/voice/volume', 10],

        ['if_session_data_eq', '/sd/val', 1, 'set', '/sd/cond1', 'fail'],
        ['if_session_data_eq', '/sd/value/x/y', 'xxx', 'set', '/sd/cond2', 'ok'],
        ['if_session_data_ne', '/sd/ne_val', 1, 'set', '/sd/ne_cond1', 'fail'],
        ['if_session_data_ne', '/sd/ne_value/x/y', 'xxx', 'set', '/sd/ne_cond2', 'ok'],
        ['if_session_data_ne', '/sd/ne_value/x/z', 'xxx', 'set', '/sd/ne_cond3', 'ok'],
        ['if_session_data_in', '/sd/value/x/y', ['xxx', 'yyy'], 'set', '/sd/cond3', 'ok'],
        ['if_session_data_in', '/sd/value/x/y', ['zzz', 'yyy'], 'set', '/sd/cond4', 'fail'],
        ['if_session_data_like', '/sd/topic', '.*map.*', 'set', '/sd/cond5', 'ok'],
        ['if_session_data_like', '/sd/topic', '.*general*', 'set', '/sd/cond6', 'fail'],

        ['if_has_payload', '/sd/gender', 'set', '/sdp/voice/gender', 'female'],
        ['if_has_payload', '/sd/volume', 'set', '/sdp/voice/volume', 10],

        ['if_payload_eq', '/sd/val', 1, 'set', '/sdp/cond1', 'fail'],
        ['if_payload_eq', '/sd/value/x/y', 'xxx', 'set', '/sdp/cond2', 'ok'],
        ['if_payload_ne', '/sd/ne_val', 1, 'set', '/sdp/ne_cond1', 'fail'],
        ['if_payload_ne', '/sd/ne_value/x/y', 'xxx', 'set', '/sdp/ne_cond2', 'ok'],
        ['if_payload_ne', '/sd/ne_value/x/z', 'xxx', 'set', '/sdp/ne_cond3', 'ok'],
        ['if_payload_in', '/sd/value/x/y', ['xxx', 'yyy'], 'set', '/sdp/cond3', 'ok'],
        ['if_payload_in', '/sd/value/x/y', ['zzz', 'yyy'], 'set', '/sdp/cond4', 'fail'],
        ['if_payload_like', '/sd/topic', '.*map.*', 'set', '/sdp/cond5', 'ok'],
        ['if_payload_like', '/sd/topic', '.*general*', 'set', '/sdp/cond6', 'fail'],

        # compare 2.1.3 with sample
        ['if_payload_loose_version_gt', '/version', '2.2.2', 'set', '/sdp/loose_version_cond1', True],
        ['if_payload_loose_version_gt', '/version', '3', 'set', '/sdp/loose_version_cond2', True],
        ['if_payload_loose_version_gt', '/version', '2.1.3', 'set', '/sdp/loose_version_cond3', True],
        ['if_payload_loose_version_gt', '/version', '2.2', 'set', '/sdp/loose_version_cond4', True],
        ['if_payload_loose_version_gt', '/version', '2.02', 'set', '/sdp/loose_version_cond5',  True],
        ['if_payload_loose_version_gt', '/version', '2.1.2', 'set', '/sdp/loose_version_cond6', True],
        ['if_payload_loose_version_gt', '/version', '1.3.4', 'set', '/sdp/loose_version_cond7', True],
        ['if_payload_loose_version_gt', '/version', '2.0.2', 'set', '/sdp/loose_version_cond8', True],
        ['if_payload_loose_version_ge', '/version', '2.1.3', 'set', '/sdp/loose_version_cond9', True],
        ['if_payload_loose_version_ge', '/version', '2.1.4', 'set', '/sdp/loose_version_cond10', True],
        ['if_payload_loose_version_ge', '/version', '2.1.1', 'set', '/sdp/loose_version_cond11', True],
        ['if_payload_loose_version_lt', '/version', '2.1.2', 'set', '/sdp/loose_version_cond12', True],
        ['if_payload_loose_version_lt', '/version', '2.1.4', 'set', '/sdp/loose_version_cond13', True],
        ['if_payload_loose_version_le', '/version', '2.1.2', 'set', '/sdp/loose_version_cond14', True],
        ['if_payload_loose_version_le', '/version', '2.1.3', 'set', '/sdp/loose_version_cond15', True],
        ['if_payload_loose_version_le', '/version', '2.1.4', 'set', '/sdp/loose_version_cond16', True],

        ['if_payload_in', '/test_in', ['asfdas', 'azaza'], 'set', '/test_in1', 'ok'],
        ['if_payload_in', '/test_in', ['asfdas', 'azqaza'], 'set', '/test_in2', 'fail'],

        ['if_has_staff_login', 'set', '/staff1', 'ok'],
        ['if_staff_login_eq', 'vasya', 'set', '/staff2', 'ok'],
        ['if_staff_login_in', ['imperator', 'vasya'], 'set', '/staff3', 'ok'],

        ['if_staff_login_eq', 'petya', 'set', '/staff4', 'fail'],
        ['if_staff_login_in', ['petya', 'imperator'], 'set', '/staff5', 'fail'],

        ['import_macro', '.macro_import', 'azaza']
    ], {'azaza': ['how_about_no']})
    event = Event({
        'header': {
            'namespace': 'ASR',
            'name': 'Recognize',
            'messageId': '123'
        },
        'payload': {
            'a': 'text-replaces-to-dict',
            'del': {
                'del/': {
                    'del@': 1,
                }
            },
            'D': {
                'E': {
                    'L2': 1,
                    'L3': 1,
                    'L4': 1,
                    'L1': 'xxx'
                }
            },
            'asr': {
                'f0': 1,
                'fy': 'here',
            },
            'voice': {
                'gender': 'male',
            },
            'val': 1,
            'chain': {
                'val1': 42,
                'val2': 'str',
            },
            'value': {
                'x': {
                    'w': 'xadfsafax',
                    'z': 'xzfsafay',
                },
            },
            'set_if_none_case': {
                'a': 2,
                'b': 4,
            },
            'experiments1': ['a'],
            'experiments2': [],
            'experiments3': "qwe",
            'experiments': {
                'experiments5': ['q', 'w'],
                'experiments6': ['q', 'w']
            },
            'experiments7': ['a', 'b'],
            'version': '2.1.3',
            'test_in': 'azaza'
        }
    })
    assert(event_patcher.useful())
    session_data = {
        'sd': {
            'value': {
                'x': {
                    'y': 'xxx',
                },
            },
            'ne_val': 1,
            'ne_value': {
                'x': {
                    'z': 'yyy'
                }
            },
            'gender': None,
            'topic': "mapsyari"
        }
    }
    event_patcher.patch(event, session_data, staff_login='vasya')

    assert(event.payload['macro_import'] == ['how_about_no'])

    assert(event.payload['test_in1'] == 'ok')
    assert('test_in2' not in event.payload)
    # check setting value
    assert(event.payload['a']['b']['c1'] == 1)
    assert(event.payload['a']['b']['c2'] == 1)
    assert(event.payload['a']['b']['c3'] == 1)
    assert(event.payload['a']['b']['c4'] == 1)
    assert(event.payload['a']['b']['c5'] == 1)
    assert(event.payload['a']['b']['c6'] == [])
    assert(event.payload['a']['b']['c7'] == '1')
    assert(event.payload['set']['/set']['@set'] == 1)

    assert(event.payload['set_if_none_case']['a'] == 2)
    assert(event.payload['set_if_none_case']['c'] == 'ok')

    # check appending value
    assert(event.payload['experiments1'] == ['a', 'b'])
    assert(event.payload['experiments2'] == ['c'])
    assert(event.payload['experiments3'] == 'qwe')
    assert(event.payload['experiments4'] == ['d'])
    assert(event.payload['experiments']['experiments5'] == ['q', 'w', 'e'])
    assert(event.payload['experiments']['experiments6'] == ['q', 'w', 'e', 't'])
    assert(event.payload['experiments7'] == ['a', 'b', 'f', 'g'])
    assert(event.payload['experiments8'] == ['a', 's'])

    # check deleting value
    assert('del@' not in event.payload['del']['del/'])
    assert('L1' not in event.payload['D']['E'])
    assert('L2' not in event.payload['D']['E'])
    assert('L3' not in event.payload['D']['E'])
    assert('L4' not in event.payload['D']['E'])

    # check filtering by event type
    assert(event.payload['asr']['f1']['f2'] == 'some_value')
    # check not setting value for wrong event type
    assert('f3' not in event.payload['asr']['f1'])
    # check not destruction values near command 'set' path
    assert(event.payload['asr']['f0'] == 1)

    # check updating only exist field
    assert(event.payload['voice']['gender'] == 'female')
    assert('volume' not in event.payload['voice'])

    # check update by condition with exist param with given value
    assert(event.payload['cond1'] == 'ok')
    assert('cond2' not in event.payload)
    assert(event.payload['cond3'] == 'ok')
    assert('cond4' not in event.payload)

    # check update by condition with loose version
    assert(event.payload['sdp'].get('loose_version_cond1') is None)
    assert(event.payload['sdp'].get('loose_version_cond2') is None)
    assert(event.payload['sdp'].get('loose_version_cond3') is None)
    assert(event.payload['sdp'].get('loose_version_cond4') is None)
    assert(event.payload['sdp'].get('loose_version_cond5') is None)
    assert(event.payload['sdp'].get('loose_version_cond6') is True)
    assert(event.payload['sdp'].get('loose_version_cond7') is True)
    assert(event.payload['sdp'].get('loose_version_cond8') is True)
    assert(event.payload['sdp'].get('loose_version_cond9') is True)
    assert(event.payload['sdp'].get('loose_version_cond10') is None)
    assert(event.payload['sdp'].get('loose_version_cond11') is True)
    assert(event.payload['sdp'].get('loose_version_cond12') is None)
    assert(event.payload['sdp'].get('loose_version_cond13') is True)
    assert(event.payload['sdp'].get('loose_version_cond14') is None)
    assert(event.payload['sdp'].get('loose_version_cond15') is True)
    assert(event.payload['sdp'].get('loose_version_cond16') is True)

    # check chain with few condition
    assert(event.payload['chain']['newval'] == 'ok')

    # check updating only exist field
    assert(event.payload['sd']['voice']['gender'] == 'female')
    assert('volume' not in event.payload['sd']['voice'])

    # check update by condition with exist param with given value
    assert(event.payload['sd']['cond2'] == 'ok')
    assert(event.payload['sd']['cond3'] == 'ok')
    assert('cond1' not in event.payload['sd'])
    assert('cond4' not in event.payload['sd'])
    assert('ne_cond1' not in event.payload['sd'])
    assert(event.payload['sd']['ne_cond2'] == 'ok')
    assert(event.payload['sd']['ne_cond3'] == 'ok')

    assert(event.payload['sd']['cond5'] == 'ok')
    assert('cond6' not in event.payload['sd'])

    # same shit different place
    assert(event.payload['sdp']['voice']['gender'] == 'female')
    assert('volume' not in event.payload['sdp']['voice'])

    assert(event.payload['sdp']['cond2'] == 'ok')
    assert(event.payload['sdp']['cond3'] == 'ok')
    assert('cond1' not in event.payload['sdp'])
    assert('cond4' not in event.payload['sdp'])
    assert('ne_cond1' not in event.payload['sdp'])
    assert(event.payload['sdp']['ne_cond2'] == 'ok')
    assert(event.payload['sdp']['ne_cond3'] == 'ok')

    assert(event.payload['sdp']['cond5'] == 'ok')
    assert('cond6' not in event.payload['sdp'])
    # check staff experiments
    assert(event.payload['staff1'] == 'ok')
    assert(event.payload['staff2'] == 'ok')
    assert(event.payload['staff3'] == 'ok')
    assert('staff4' not in event.payload)
    assert('staff5' not in event.payload)

    event.payload = {}

    event_patcher.patch(event, session_data, staff_login=None)
    assert('staff1' not in event.payload)
    assert('staff2' not in event.payload)
    assert('staff3' not in event.payload)
    assert('staff4' not in event.payload)
    assert('staff5' not in event.payload)


def test_tricky_patchers():
    Logger.init('unittest', True)

    def create_event_patcher(flags):
        return EventPatcher(flags, {})

    event = Event({
        'header': {
            'namespace': 'ASR',
            'name': 'Recognize',
            'messageId': '123'
        },
        'payload': {
            'a': 0,
        }
    })

    event_patcher = create_event_patcher([
        ['append', '.request.experiments', 'a1'],
    ])

    event_patcher.patch(event, {}, hide_exceptions=False)
    assert(event.payload['request']['experiments'] == dict(a1='1'))

    event_patcher = create_event_patcher([
        ['extend', '.request.experiments', ['a2', 'a3']],
    ])
    event_patcher.patch(event, {}, hide_exceptions=False)
    assert(event.payload['request']['experiments'] == dict(a1='1', a2='1', a3='1'))

    event.payload['request']['experiments'] = {}
    event_patcher = EventPatcher(
        [
            ['import_macro', '.request.experiments', 'm1']
        ],
        {
            'm1': ['b1', 'b2', 'b3'],
        },
    )
    event_patcher.patch(event, {}, hide_exceptions=False)
    assert(event.payload['request']['experiments'] == dict(b1='1', b2='1', b3='1'))


def test_event_patcher_error_handling():
    Logger.init('unittest', True)

    def create_event_patcher(flags):
        return EventPatcher(flags, {})

    event = Event({
        'header': {
            'namespace': 'ASR',
            'name': 'Recognize',
            'messageId': '123'
        },
        'payload': {
            'a': 0,
        }
    })

    event_patcher = create_event_patcher([
        ['set', '.a', 1],
        ['badcommand'],
    ])

    with pytest.raises(Exception):
        event_patcher.patch(event, {}, hide_exceptions=False)

    # check restoring original payload after fail
    assert(event.payload['a'] == 0)

    def test_bad_filter(flt):
        event_patcher = create_event_patcher([
            flt,
        ])
        assert(event_patcher.useful())
        with pytest.raises(Exception):
            event_patcher.patch(event, {}, hide_exceptions=False)

    test_bad_filter(['set', 'a', 1])
    test_bad_filter(['set', '.a'])
    test_bad_filter(['del', 'a'])
    test_bad_filter(['del'])
    test_bad_filter(['aaa'])
    test_bad_filter(['if_event_type'])
    test_bad_filter(['if_has_payload'])
    test_bad_filter(['if_payload_eq'])
    test_bad_filter(['if_payload_eq', 'a', 0])
    test_bad_filter(['if_payload_eq', '.a'])


def test_multi_patcher():
    Logger.init('unittest', True)

    mpatcher = MultiPatcher()
    mpatcher.add_patcher(EventPatcher([
        ['if_payload_eq', '/val', 1, 'set', '/cond1', 'ok'],
    ], {}))
    mpatcher.add_patcher(EventPatcher([
        ['if_payload_eq', '/val2', 2, 'set', '/cond2', 'ok'],
    ], {}))
    event = Event({
        'header': {
            'namespace': 'ASR',
            'name': 'Recognize',
            'messageId': '123'
        },
        'payload': {
            'val': 1,
            'val2': 2,
        }
    })
    assert(mpatcher.useful())
    session_data = {
        'sd': {
            'value': {
                'x': {
                    'y': 'xxx',
                },
            },
            'gender': None,
        }
    }
    mpatcher.patch(event, session_data)

    # check update by condition with exist param with given value
    assert(event.payload['cond1'] == 'ok')
    assert(event.payload['cond2'] == 'ok')
