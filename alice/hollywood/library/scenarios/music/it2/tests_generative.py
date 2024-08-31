import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries
from alice.hollywood.library.python.testing.it2.input import server_action, voice, callback, Scenario
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    assert_audio_play_directive,
    get_callback_from_reset_add_effect,
    get_first_track_id,
    prepare_server_action_data,
)
from alice.hollywood.library.scenarios.music.proto.music_arguments_pb2 import TMusicArguments  # noqa
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState, ERepeatType, TContentId  # noqa
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from alice.megamind.protos.scenarios.directives_pb2 import TAudioPlayDirective, TPlayerRewindDirective  # noqa
from hamcrest import assert_that, has_entries, empty, is_not, contains


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

logger = logging.getLogger(__name__)

EXPERIMENTS_GENERATIVE = [
    'hw_music_thin_client_generative',
    'bg_fresh_granet',
]

GENERATIVE_RESPONSES_RE = r'.*(настроит на нужный лад|вдохнов|немного драйва|Нейро музык|расслабл.).*'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS_GENERATIVE)
class TestsBaseGenerative:
    pass


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsGenerative(TestsBaseGenerative):

    @pytest.mark.parametrize('command, output_speech_re', [
        pytest.param('включи нейромузыку', GENERATIVE_RESPONSES_RE, id='default'),
        pytest.param('включи нейронную музыку для сосредоточения', r'.*(настроит на нужный лад|вдохнов).*', id='focus'),
        pytest.param('включи бодрящую нейронную музыку', r'.*(немного драйва|Нейромузык).*', id='energy'),
        pytest.param('включи музыку для сна', r'.*расслабл.*', marks=pytest.mark.skip(reason='waiting for granet'), id='relax'),
    ])
    def test_play_generative(self, alice, command, output_speech_re):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(output_speech_re, layout.OutputSpeech)
        assert_audio_play_directive(layout.Directives, is_hls=True)
        return str(r)

    @pytest.mark.parametrize('command, text', [
        pytest.param('перемотай назад на 30 секунд', 'Простите, я не могу перемотать нейромузыку.', id='rewind_backward'),
        pytest.param('перемешай музыку', 'Простите, я не могу перемешать нейромузыку.', id='shuffle'),
        pytest.param('поставь на повтор', 'Простите, не могу поставить на повтор нейромузыку.', id='repeat'),
        pytest.param('включи заново', 'Простите, я не могу повторить нейромузыку.', id='replay'),
    ])
    def test_play_generative_music_unsupported_commands(self, alice, command, text):
        r = alice(voice('включи нейромузыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(GENERATIVE_RESPONSES_RE, layout.OutputSpeech)
        assert_audio_play_directive(layout.Directives, is_hls=True)

        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == text
        return str(r)  # XXX(sparkle): added for HOLLYWOOD-948

    def test_play_generative_what_is_playing(self, alice):
        r = alice(voice('включи нейромузыку для сосредоточения'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        assert re.match(r'.*(настроит на нужный лад|вдохнов).*', r.continue_response.ResponseBody.Layout.OutputSpeech)

        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        assert re.match(r'.*Вдохновение.*', r.run_response.ResponseBody.Layout.OutputSpeech)
        assert not r.run_response.ResponseBody.Layout.Directives
        return str(r)

    @pytest.mark.parametrize('object_id, object_type, title, is_hls', [
        pytest.param('generative:focus', 'Generative', 'Вдохновение', True, id='focus'),
    ])
    def test_play_generative_with_tsf(self, alice, object_id, object_type, title, is_hls):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'play_single_track': {
                        'bool_value': True
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(layout.Directives, title=title, is_hls=is_hls)

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        assert not reset_add.Effects


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsGenerativeCallbacks(TestsBaseGenerative):

    def test_play_generative_on_started(self, alice):
        '''
        Когда включается генеративный стрим, то никакого генеративного
        фидбека дополнительно отправлять не нужно
        '''
        r = alice(voice('включи нейромузыку'))
        assert r.scenario_stages() == {'run', 'continue'}

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': {},
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives, is_hls=True)
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

    def test_play_generative_on_stopped(self, alice):
        '''
        Когда генеративный стрим ставится на паузу, то создается генеративный фидбек StreamPause на on_stopped
        '''
        r = alice(voice('включи нейромузыку для сосредоточения'))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = directives[0].AudioPlayDirective
        assert audio_play.Stream.OffsetMs == 0, "Проигрывание начинается с начала"
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_stopped': has_only_entries({
                'name': 'music_thin_client_on_stopped',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'generativeFeedbackEvent': has_only_entries({
                            'type': 'StreamPause',
                            'generativeStationId': 'generative:focus',
                            'streamId': is_not(empty()),
                        })
                    }))
                })
            })
        }))

        alice.skip(60)
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        assert r.scenario_stages() == {'run'}

    def test_play_generative_on_started_continue(self, alice):
        '''
        Когда генеративный стрим начинает проигрываться после паузы,
        то сначала мы идем в StreamPlay с проверкой, что перезагружать стрим не нужно,
        а если не нужно, то присылаем директиву audio_play, как обычно
        '''
        r = alice(voice('включи нейромузыку для сосредоточения'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        audio_play = layout.Directives[0].AudioPlayDirective
        first_track_id = audio_play.Stream.Id
        assert audio_play.Stream.OffsetMs == 0, "Проигрывание начинается с начала"

        alice.skip(60)
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'apply'}
        audio_play = r.apply_response.ResponseBody.Layout.Directives[0].AudioPlayDirective
        assert first_track_id == audio_play.Stream.Id, 'Это должен быть тот же стрим'
        first_track_id = audio_play.Stream.Id
        assert audio_play.Stream.OffsetMs == 0, "Для Генеративной музыки offset всегда 0 (HOLLYWOOD-453)"
        callbacks = r.apply_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'generativeFeedbackEvent': has_only_entries({
                            'type': 'StreamPlay',
                            'generativeStationId': 'generative:focus',
                            'streamId': is_not(empty()),
                        })
                    }))
                })
            })
        }))

    @pytest.mark.experiments('music_thin_client_generative_force_reload_on_stream_play')
    def test_play_generative_on_started_continue_force_reload(self, alice):
        '''
        Когда генеративный стрим начинает проигрываться после долгой паузы (когда стрим протух),
        то StreamPlay вернет нам информацию, что нужно перезагрузить стрим. Мы отправим пустой ответ с коллбеком
        'music_thin_client_next' и в итоге перезагрузим стрим.
        '''
        r = alice(voice('включи нейромузыку для сосредоточения'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        audio_play = layout.Directives[0].AudioPlayDirective
        first_track_id = audio_play.Stream.Id
        assert audio_play.Stream.OffsetMs == 0, "Проигрывание начинается с начала"

        alice.skip(60)
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        next_callback = get_callback_from_reset_add_effect(r.apply_response.ResponseBody.StackEngine.Actions[0].ResetAdd,
                                                           callback_name='music_thin_client_next')

        r = alice(callback(name=next_callback['name'], payload=next_callback['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives, is_hls=True)
        assert first_track_id != audio_play.Stream.Id, 'Это должен быть другой стрим'
        first_track_id = audio_play.Stream.Id
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': {},
            })
        }))
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

    def test_play_generative_on_started_dislike(self, alice):
        '''
        После команды "дизлайк" отправляется генеративный фидбек timestampDislike
        с колбеком 'music_thin_client_next', который сразу же попросит
        себе новый генеративный поток и включит его
        '''
        r = alice(voice('включи нейромузыку для сосредоточения'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Поняла, сейчас алгоритмы напишут что-то другое.',
            'Окей, алгоритмы Нейро музыки уже учли ваш дизлайк.',
            'О вкусах не спорят. Попрошу Нейро музыку подстроиться под ваш.',
        ]

        next_callback = get_callback_from_reset_add_effect(r.apply_response.ResponseBody.StackEngine.Actions[0].ResetAdd,
                                                           callback_name='music_thin_client_next')

        r = alice(callback(name=next_callback['name'], payload=next_callback['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives, is_hls=True)
        assert first_track_id != audio_play.Stream.Id
        first_track_id = audio_play.Stream.Id
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': {},
            })
        }))
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

    def test_play_generative_on_started_skip(self, alice):
        '''
        После команды "пропусти" отправляется генеративный фидбек timestampSkip
        с колбеком 'music_thin_client_next', который сразу же попросит
        себе новый генеративный поток и включит его
        '''
        r = alice(voice('включи нейромузыку для сосредоточения'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('пропусти'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Пропускаю.'
        next_callback = get_callback_from_reset_add_effect(r.apply_response.ResponseBody.StackEngine.Actions[0].ResetAdd,
                                                           callback_name='music_thin_client_next')

        r = alice(callback(name=next_callback['name'], payload=next_callback['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives, is_hls=True)
        assert first_track_id != audio_play.Stream.Id
        first_track_id = audio_play.Stream.Id
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': {},
            })
        }))
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}


@pytest.mark.supported_features('music_sdk_client')
@pytest.mark.experiments('internal_music_player')
@pytest.mark.parametrize('surface', [surface.searchapp, surface.navi, surface.legatus, surface.dexp])
class TestsGenerativeUnsupportedSurfaces(TestsBaseGenerative):

    def test_play_generative(self, alice):
        r = alice(voice('включи нейромузыку'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Простите, не могу включить нейромузыку в этом приложении.'
        return str(r)
