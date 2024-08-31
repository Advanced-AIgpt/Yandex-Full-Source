import logging
import re

import pytest
import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

logger = logging.getLogger(__name__)

DEVICE_STATE_LETOV = {
    'music': {
        'currently_playing': {
            'track_info': {
                # https://music.yandex.ru/album/5829983/track/43741729
                'artists': [{'id': 42528}],
                'albums': [{'id': 5829983}],
                'id': '43741729'
            }
        }
    }
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
class Tests:

    @pytest.mark.experiments('tts_domain_music')
    def test_play_albom_master_of_puppets(self, alice):
        r = alice(voice('включи альбом master of puppets'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('new_music_radio_nlg')
    def test_play_music_for_jogging(self, alice):
        r = alice(voice('включи музыку для бега'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('medium_ru_explicit_content')
    def test_play_novelties(self, alice):
        r = alice(voice('включи новинки'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_play_playlist_deftones(self, alice):
        r = alice(voice('включи подборку deftones'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_play_queen(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('new_music_radio_nlg')
    def test_play_rock(self, alice):
        r = alice(voice('включи рок'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('new_music_radio_nlg')
    def test_play_sad_music(self, alice):
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_play_song_jamming_of_bob_marley(self, alice):
        r = alice(voice('включи песню джамин боба марли'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('medium_ru_explicit_content')
    def test_play_song_taet_led(self, alice):
        r = alice(voice('включи песню тает лед'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('medium_ru_explicit_content')
    def test_recommend_me_music(self, alice):
        r = alice(voice('порекомендуй мне музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_filters_not_applied(self, alice):
        r = alice(voice('включи веселый рок для бега'))  # не применит фильтр run
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_fairy_tale_album(self, alice):
        r = alice(voice('включи сказку заходера'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_audiobook_album(self, alice):
        r = alice(voice('включи аудиокнигу двенадцать стульев'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_podcast_album(self, alice):
        r = alice(voice('включи подкаст про лайфхакер'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_playlist_of_the_day(self, alice):
        r = alice(voice('включи плейлист дня'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_playlist_with_alice_shots(self, alice):
        r = alice(voice('включи плейлист с твоими шотами'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.parametrize('object_id, object_type, first_track_id, start_from_track_id,'
                             'track_offset_index, offset, alarm_id, disable_nlg', [
        pytest.param('38646012', 'Track', '38646012', None, None, 0, None, True, id='track'),
        pytest.param('211604', 'Album', '38646012', '38646012', None, 0, None, True, id='album_start_from_track_id'),
        pytest.param('211604', 'Album', '2142676', None, 9, 0, None, True, id='album_track_offset_index'),
        pytest.param('38646012', 'Track', '38646012', None, None, 2.345, None, True, id='track_offset'),
        pytest.param('38646012', 'Track', '38646012', None, None, 0, 'sample_alarm_id', True, id='track_alarm_id'),
        pytest.param('38646012', 'Track', '38646012', None, None, 0, None, False, id='track_enable_nlg')
    ])
    def test_music_play_semantic_frame_with_object_id(self, alice, object_id, object_type, first_track_id,
                                                      start_from_track_id, track_offset_index, offset, alarm_id,
                                                      disable_nlg):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        if start_from_track_id is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['start_from_track_id'] = {
                'string_value': start_from_track_id,
            }
        if track_offset_index is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['track_offset_index'] = {
                'num_value': track_offset_index
            }
        if offset != 0:
            payload['typed_semantic_frame']['music_play_semantic_frame']['offset_sec'] = {
                'double_value': offset
            }
        if alarm_id is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['alarm_id'] = {
                'string_value': alarm_id
            }
        if disable_nlg is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['disable_nlg'] = {
                'bool_value': disable_nlg
            }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        if not disable_nlg:
            assert layout.OutputSpeech == 'Включаю'
        else:
            assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        music_play = directives[0].MusicPlayDirective
        assert music_play.SessionId.strip()
        assert music_play.FirstTrackId == first_track_id
        assert music_play.Offset == offset
        if alarm_id is not None:
            assert music_play.AlarmId == alarm_id

    @pytest.mark.parametrize('radio_station_id', [
        pytest.param('mood:beautiful', id='mood'),
        pytest.param('track:12345', id='track')
    ])
    def test_music_play_semantic_frame_radio(self, alice, radio_station_id):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': radio_station_id,
                    },
                    'object_type': {
                        'enum_value': 'Radio',
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
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')

    def test_music_play_semantic_frame_from(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '38646012',
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'from': {
                        'string_value': 'from_value'
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout

        assert not layout.OutputSpeech
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')

    def test_music_play_semantic_frame_radio_from(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'mood:beautiful',
                    },
                    'object_type': {
                        'enum_value': 'Radio',
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'from': {
                        'string_value': 'from_value'
                    }
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
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')

    def test_music_play_semantic_frame_with_object_id_generative_without_exp(self, alice):
        '''
        Return irrelevant for generative without experiment flags
        '''

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'generative:lucky',
                    },
                    'object_type': {
                        'enum_value': 'Generative',
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

        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech in {
            'К сожалению, у меня нет такой музыки.',
            'Была ведь эта музыка у меня где-то... Не могу найти, простите.',
            'Как назло, именно этой музыки у меня нет.',
            'У меня нет такой музыки, попробуйте что-нибудь другое.',
            'Я не нашла музыки по вашему запросу, попробуйте ещё.',
        }

    def test_shuffle_repeat_all(self, alice):
        first_track_id = '2774870'
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '298151',
                    },
                    'object_type': {
                        'enum_value': 'Album',
                    },
                    'start_from_track_id': {
                        'string_value': '2774870'
                    },
                    'order': {
                        'order_value': 'shuffle'
                    },
                    'repeat': {
                        'repeat_value': 'All'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        music_play = directives[0].MusicPlayDirective
        assert music_play.SessionId.strip()
        assert music_play.FirstTrackId == first_track_id

    def test_play_fairytale_aibolit(self, alice):
        r = alice(voice('прочитай доктор айболит'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_fixlist_track_on_demand(self, alice):
        r = alice(voice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response,
                                               output_speech='Включаю трек по запросу: Дора, Не исправлюсь.')

    @pytest.mark.experiments('yamusic_audiobranding_score=1')
    def test_yamusic_audiobranding(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    def test_ambient_sounds_nlg(self, alice):
        r = alice(voice('включи шум дождя'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('hw_music_reverse_source_text_mapping')
    def test_source_text_mapping(self, alice):
        r = alice(voice('включи веселый рок 80-х для бега по фински'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.experiments('hollywood_music_play_less')
    def test_music_play_less(self, alice):
        r = alice(voice('слишком много рэпа'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        # The feature 'play less' is not fully implemented yet,
        # this test just checks that a certain NLG stub is returned,
        # see https://a.yandex-team.ru/review/1434818/files/1#file-0-43693031
        expected_texts = [
            'Упс. Я так не умею, но можете попросить меня включить музыку по настроению или жанру.',
            'Извините, но я не могу включить то, что вы просите. Зато умею включать музыку по жанрам: рок, джаз, рэп, электроника.',
            'Простите, я бы с радостью, но так не умею. Давайте послушаем что-нибудь ещё.',
        ]
        assert layout.OutputSpeech in expected_texts
        assert layout.Cards[0].Text in expected_texts
        assert len(layout.Cards) == 1
        assert not layout.Directives

    def test_music_play_sf_slots(self, alice):
        # check that new slots do not break anything in the thick music client
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'search_text': {
                        'string_value': 'включи альбом the dark side of the moon',
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
                    'track_offset_index': {
                        'num_value': 1
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test',
            },
        }))
        assert r.scenario_stages() == {'run', 'continue'}
        return str(r)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface, directive', [
    pytest.param(surface.station, 'MusicPlayDirective', id='station'),
    pytest.param(surface.searchapp, 'OpenUriDirective', marks=pytest.mark.supported_features('music_sdk_client'),
                 id='searchapp'),
])
class TestsPlaylists:

    def test_playlist_verification_personal(self, alice, directive, request):
        if request.node.callspec.id == 'station':
            request.applymarker(pytest.mark.xfail(reason='Test is dead after recanonization'))
        r = alice(voice('включи плейлист абацаба'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert len(r.continue_response.ResponseBody.Layout.Directives) == 1
        assert r.continue_response.ResponseBody.Layout.Directives[0].HasField(directive)
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech

    def test_playlist_verfication_verified_user(self, alice, directive):
        r = alice(voice('включи 100 суперхитов'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert len(r.continue_response.ResponseBody.Layout.Directives) == 1
        assert r.continue_response.ResponseBody.Layout.Directives[0].HasField(directive)
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech

    def test_playlist_verification_unverified_user(self, alice, directive):
        r = alice(voice('включи отстойную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert len(r.continue_response.ResponseBody.Layout.Directives) == 1
        assert r.continue_response.ResponseBody.Layout.Directives[0].HasField(directive)
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Нашла что-то подходящее среди плейлистов других пользователей. Включаю \"Топ идиотских песен\".',
            'Вот что я нашла среди плейлистов других пользователей. Включаю \"Топ идиотских песен\".',
        ]

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_playlist_verification_unverified_user_shuffled(self, alice, directive):
        r = alice(voice('включи армейские песни вперемешку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert len(r.continue_response.ResponseBody.Layout.Directives) == 1
        assert r.continue_response.ResponseBody.Layout.Directives[0].HasField(directive)
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Нашла что-то подходящее среди плейлистов других пользователей. Включаю \"Армейские песни\" вперемешку.',
            'Вот что я нашла среди плейлистов других пользователей. Включаю \"Армейские песни\" вперемешку.',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
class TestsSlotsCombo:

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_ondemand_with_radio_slot(self, alice):
        r = alice(voice('включи ac/dc для тренировки'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')

        event = next(filter(lambda event: event.HasField('MusicEvent'),
                     r.continue_response.ResponseBody.AnalyticsInfo.Events))
        assert event.MusicEvent.AnswerType == TMusicEvent.EAnswerType.Artist


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.device_state(device_config={
    'content_settings': 'medium',
})
class TestsSFContentSettings:
    def test_first_track_id(self, alice):
        # With medium(default) content settings playback should start from requested track
        first_track_id = '19781921'
        expected_first_track_id = '19781921'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '44612130:1000',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'start_from_track_id': {
                        'string_value': first_track_id
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        music_play = directives[0].MusicPlayDirective
        assert music_play.SessionId.strip()
        assert music_play.FirstTrackId == expected_first_track_id


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.device_state(device_config={
    'content_settings': 'children',
})
class TestsSFChildrenContentSettings:
    def test_first_track_id(self, alice):
        # With children content settings playback should start from requested track
        first_track_id = '19781921'
        expected_first_track_id = '34826282'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '44612130:1000',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'start_from_track_id': {
                        'string_value': first_track_id
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        music_play = directives[0].MusicPlayDirective
        assert music_play.SessionId.strip()
        assert music_play.FirstTrackId == expected_first_track_id


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.device_state(device_config={
    'content_settings': 'safe',
})
class TestsSFSafeContentSettings:
    def test_first_track_id(self, alice):
        # With children content settings playback should start from requested track
        first_track_id = '19781921'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '44612130:1000',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'start_from_track_id': {
                        'string_value': first_track_id
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Лучше послушай эту музыку вместе с родителями.'
        assert len(layout.Directives) == 0


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class TestsMultiroom:

    ROOM_ID_ALL = '__all__'

    # surface.station has 'multiroom' feature in its app_preset by default
    # We just want to declare it explicitly
    @pytest.mark.supported_features('multiroom')
    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_multiroom_hit(self, alice):
        r = alice(voice('включи мощный хит везде'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю'
        directives = layout.Directives
        assert len(directives) == 2

        start_multiroom = directives[0].StartMultiroomDirective
        assert start_multiroom.RoomId == TestsMultiroom.ROOM_ID_ALL

        music_play = directives[1].MusicPlayDirective
        assert music_play.SessionId
        assert music_play.RoomId == TestsMultiroom.ROOM_ID_ALL

    @pytest.mark.parametrize('surface', [surface.dexp])
    def test_multiroom_hit_unsupported(self, alice):
        '''
        surface.dexp has no 'multiroom' feature in its app_preset. NOTE: This may change in the future
        '''
        r = alice(voice('включи мощный хит везде'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Я ещё не научилась играть музыку на разных устройствах одновременно'
        directives = layout.Directives
        assert len(directives) == 1

        music_play = directives[0].MusicPlayDirective
        assert music_play.SessionId
        assert not music_play.RoomId


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(
    'music_force_show_first_track',
    'hollywood_music_play_anaphora',
)
class TestsAnaphora:

    def test_anaphora_irrelevant_no_device_state(self, alice):
        # should answer with music_play, not music_play_anaphora
        r = alice(voice('включи этого артиста'))
        return str(r)

    @pytest.mark.device_state(DEVICE_STATE_LETOV)
    def test_anaphora_artist(self, alice):
        r = alice(voice('включи этого артиста'))
        return str(r)

    @pytest.mark.device_state(DEVICE_STATE_LETOV)
    def test_anaphora_album(self, alice):
        r = alice(voice('включи этот альбом'))
        return str(r)

    @pytest.mark.device_state(DEVICE_STATE_LETOV)
    def test_anaphora_need_similar(self, alice):
        r = alice(voice('включи похожее на этот трек'))
        return str(r)

    @pytest.mark.device_state(DEVICE_STATE_LETOV)
    def test_anaphora_shuffle(self, alice):
        r = alice(voice('включи этот альбом в случайном порядке'))
        return str(r)

    @pytest.mark.device_state(DEVICE_STATE_LETOV)
    def test_anaphora_repeat(self, alice):
        r = alice(voice('включи этот альбом на репите'))
        return str(r)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(
    'bg_enable_music_play_experimental',
)
class TestsReask:

    def test_state_negative(self, alice):
        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 2)

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = TScenarioState()
        r.continue_response.ResponseBody.State.Unpack(state)
        assert state.ReaskState.ReaskCount == 0
        _assert_music_play_directive_is_single(r.continue_response)

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

    def test_state_positive(self, alice):
        r = alice(voice('включи группу'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

        r = alice(voice('linkin park'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = TScenarioState()
        r.continue_response.ResponseBody.State.Unpack(state)
        assert state.ReaskState.ReaskCount == 0
        _assert_music_play_directive_is_single(r.continue_response)

        r = alice(voice('включи группу'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

    def test_reset_state(self, alice):
        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)


@pytest.mark.experiments('music_disable_reask')
class TestsDisableReask(TestsReask):

    # TODO(ardulat): test disable reask
    def test_state_negative(self, alice):
        pass

    def test_state_positive(self, alice):
        pass

    def test_reset_state(self, alice):
        pass


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
class TestsPodcasts:

    def test_podcasts_default(self, alice):
        r = alice(voice('включи подкаст'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_podcasts_child(self, alice):
        r = alice(voice('включи подкаст'))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_music_play_directive_is_single(r.continue_response)
        return str(r)


def _assert_music_play_directive_is_single(response, output_speech=None):
    layout = response.ResponseBody.Layout
    if output_speech:
        assert layout.OutputSpeech == output_speech
    directives = layout.Directives
    assert len(directives) == 1

    music_play = directives[0].MusicPlayDirective
    assert music_play.SessionId.strip()
    assert not music_play.RoomId


def _assert_reask_response(response, reask_count):
    state = TScenarioState()
    response.ResponseBody.State.Unpack(state)
    assert state.ReaskState.ReaskCount == reask_count
    layout = response.ResponseBody.Layout

    if reask_count == 1:
        assert layout.OutputSpeech in [
            'Какую <[ accented ]> песню?',
            'Какой <[ accented ]> альбом?',
            'Какого <[ accented ]> исполнителя?',
            'Не поняла. Какую <[ accented ]> песню?',
            'Не поняла. Какой <[ accented ]> альбом?',
            'Не поняла. Какого <[ accented ]> исполнителя?',
            'Кого <[ accented ]> включить?',
            'Что <[ accented ]> включить?',
        ]
    else:
        assert layout.OutputSpeech in [
            'Помедленее, я не поняла!',
            'Повторите почётче, пожалуйста!',
        ]
    assert not layout.Directives

    analytics_info = response.ResponseBody.AnalyticsInfo
    assert analytics_info.Intent == "music.reask"
    assert analytics_info.ProductScenarioName == "music"


def _assert_fairytale_reask_response(response, fairytale_reask_count):
    state = TScenarioState()
    response.ResponseBody.State.Unpack(state)
    assert state.FairytaleReaskState.FairytaleReaskCount == fairytale_reask_count
    layout = response.ResponseBody.Layout
    assert re.match('Включить вам сказку|Могу предложить сказку|Как насчёт сказки ".*" или поискать еще?', layout.OutputSpeech)
    assert not layout.Directives


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestTV:

    # smart_tv doesn't have 'music_quasar_client' feature in it's app preset
    # because features in preset are outdated. So we declare it explicitly
    @pytest.mark.supported_features('music_quasar_client')
    def test_music_play_semantic_frame(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '38646012',
                    },
                    'object_type': {
                        'enum_value': 'Track',
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

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout

        assert not layout.OutputSpeech
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')


@pytest.mark.experiments(
    'hw_music_thin_client',
    'hw_music_thin_client_playlist',
)
@pytest.mark.xfail(reason='Test is dead after recanonization')
class TestThinPlayerTV(TestTV):
    pass
