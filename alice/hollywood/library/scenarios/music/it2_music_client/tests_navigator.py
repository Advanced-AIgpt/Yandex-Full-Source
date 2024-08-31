import re
from urllib.parse import urlparse, parse_qs

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.hamcrest import non_empty_dict
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.music.it2_music_client.music_sdk_helpers import (
    check_analytics_info,
    check_is_various,
    check_vins_uri,
    check_music_sdk_uri,
    check_response,
    check_suggests_common,
    get_response,
    get_response_pyobj,
)
from hamcrest import assert_that, any_of, has_entries, contains, empty, is_not


# BASE CLASSES
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.navi])
@pytest.mark.supported_features('music_sdk_client')
@pytest.mark.experiments('internal_music_player')
class _TestsNavigatorUnauthorizedBase:
    pass


@pytest.mark.oauth(auth.YandexPlus)
class _TestsNavigatorBase(_TestsNavigatorUnauthorizedBase):
    pass


# AVAILABILITY
class _TestsNavigatorAvailabilityBase(_TestsNavigatorUnauthorizedBase):

    UNAUTHORIZED_TEXTS = [
        'Если хотите послушать музыку, авторизуйтесь, пожалуйста.',
        'Авторизуйтесь, пожалуйста, и сразу включу вам все, что попросите!',
    ]

    NO_PLUS_TEXTS = [
        'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
        'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
        'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
    ]

    @pytest.mark.parametrize('texts', [
        pytest.param(UNAUTHORIZED_TEXTS, id='unauthorized'),
        pytest.param(NO_PLUS_TEXTS, id='no_plus', marks=pytest.mark.oauth(auth.Yandex)),
    ])
    def test_smoke(self, alice, texts):
        '''
        На навигаторе не даем слушать, если пользователь неавторизован или без подписки
        '''
        r = alice(voice('включи рок'))

        # голос совпадает с текстом, но без точки в "Яндекс.Плюс"
        output_speeches = [text.replace('Яндекс.Плюс', 'Яндекс Плюс') for text in texts]

        assert_that(get_response_pyobj(r), has_entries({
            'layout': has_entries({
                'cards': contains(has_entries({
                    'text': any_of(*texts),
                })),
                'output_speech': any_of(*output_speeches),
            }),
        }))

        check_analytics_info(r, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'objects': contains(has_entries({
                'vins_error_meta': has_entries({
                    'type': 'unauthorized',
                }),
            })),
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': has_entries({
                    'source': 'music',
                }),
            })),
            'product_scenario_name': 'music',
        }))

        return str(r)


@pytest.mark.skip
class TestsNavigatorAvailabilityBass(_TestsNavigatorAvailabilityBase):
    pass


class TestsNavigatorAvailabilityHw(_TestsNavigatorAvailabilityBase):
    pass


# PLAYLISTS
class _TestsNavigatorPlaylistsBase(_TestsNavigatorBase):

    @pytest.mark.parametrize('command, name, kind, owner', [
        # редакторские плейлисты
        pytest.param(
            'поставь плейлист вечные хиты',
            'Вечные хиты',
            '1250',
            '105590476',
            id='eternal_hits',
        ),
        pytest.param(
            'включи подборку лёгкое электронное утро',
            'Лёгкое электронное утро',
            '1459',
            '103372440',
            id='electro_morning',
        ),
        pytest.param(
            'включи громкие новинки месяца',
            'Громкие новинки месяца',
            '1175',
            '103372440',
            id='loud_novelty',
        ),

        # личные плейлисты
        pytest.param(
            'включи плейлист дня',
            'Плейлист дня',
            '127167070',
            '503646255',
            id='playlist_of_the_day',
        ),
        pytest.param(
            'запусти плейлист дежавю',
            'Дежавю',
            '119307282',
            '692528232',
            id='dejavu',
        ),
        pytest.param(
            'поставь плейлист премьера',
            'Премьера',
            '121818957',
            '692529388',
            id='premiere',
        ),
    ])
    def test_playlist(self, alice, command, name, kind, owner):
        r = alice(voice(command))

        check_response(r, texts=[f'Включаю подборку \"{name}\".'], output_speeches=['Включаю'])
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': [kind],
            'owner': [owner],
        })
        check_analytics_info(r, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': non_empty_dict(),
            }), has_entries({
                'timestamp': '1579488271002000',
                'music_event': non_empty_dict(),
            })),
            'product_scenario_name': 'music',
        }))

        return str(r)

    def test_playlist_origin(self, alice):
        r = alice(voice('включи плейлист с Алисой'))

        check_response(r,
                       texts=['Я бы с радостью, но умею такое только в приложении Яндекс.Музыка, или в умных колонках.'],
                       output_speeches=['Я бы с радостью, но умею такое только в приложении Яндекс Музыка, или в умных колонках.'])

        check_vins_uri(r, expected_query_info={
            'url': ['intent://playlist/origin/?from=alice&play=1#Intent;scheme=yandexmusic;package=ru.yandex.music;'
                     'S.browser_fallback_url=https%3A%2F%2Fmusic.yandex.ru%2Fplaylist%2Forigin%2F%3Ffrom%3Dalice%26mob%3D0%26play%3D1;end']
        })
        check_analytics_info(r, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': non_empty_dict(),
            }), has_entries({
                'timestamp': '1579488271002000',
                'music_event': non_empty_dict(),
            })),
            'product_scenario_name': 'music',
        }))

        return str(r)

    def test_my_music(self, alice):
        r = alice(voice('включи мою музыку'))

        nlg_responses = [
            'Послушаем ваше любимое.',
            'Включаю ваши любимые песни.',
            'Окей. Плейлист с вашей любимой музыкой.',
            'Люблю песни, которые вы любите.',
            'Окей. Песни, которые вам понравились.',
        ]
        check_response(r, texts=nlg_responses, output_speeches=nlg_responses)

        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': ['3'],
            'owner': ['1083955728'],
        })

        check_analytics_info(r, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': non_empty_dict(),
            }), has_entries({
                'timestamp': '1579488271002000',
                'music_event': non_empty_dict(),
            })),
            'product_scenario_name': 'music',
        }))

        return str(r)

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_unverified_playlist(self, alice):
        r = alice(voice('включи армейские песни'))

        nlg_responses = [
            'Вот что я нашла среди плейлистов других пользователей. Включаю \"Армейские песни\".',
            'Нашла что-то подходящее среди плейлистов других пользователей. Включаю \"Армейские песни\".',
        ]
        check_response(r, texts=nlg_responses, output_speeches=nlg_responses)

        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': ['1023'],
            'owner': ['35990606'],
        })

        return str(r)


@pytest.mark.skip
class TestsNavigatorPlaylistsBass(_TestsNavigatorPlaylistsBase):
    pass


class TestsNavigatorPlaylistsHw(_TestsNavigatorPlaylistsBase):
    pass


# RADIO
class _TestsNavigatorRadioBase(_TestsNavigatorBase):

    @pytest.mark.parametrize('command, radio_type, radio_tag', [
        pytest.param('включи индастриал', 'genre', 'industrial', id='genre'),
        pytest.param('включи музыку девяностых', 'epoch', 'nineties', id='epoch'),
        pytest.param('включи веселую музыку', 'mood', 'happy', id='mood'),
        pytest.param('включи музыку для бега', 'activity', 'run', id='activity'),
        # pytest.param('включи итальянскую музыку', id='language'),  # TODO(sparkle): not working!
        # pytest.param('включи инструментальную музыку', id='vocal'),  # TODO(sparkle): not working!
    ])
    def test_smoke(self, alice, command, radio_type, radio_tag):
        '''
        Ответы в Bass- и Hollywood-бэкендах должны функционально совпадать
        '''
        r = alice(voice(command))

        assert_that(get_response_pyobj(r), has_entries({
            'layout': has_entries({
                'cards': contains(has_entries({
                    'text': 'Включаю.',
                })),
                'output_speech': 'Включаю',
                'directives': contains(has_entries({
                    'open_uri_directive': has_entries({
                        'name': 'music_internal_player_play',
                        'uri': is_not(empty()),
                    }),
                })),
                'suggest_buttons': contains(has_entries({
                    'action_button': has_entries({
                        'title': 'Что ты умеешь?',
                        'action_id': is_not(empty()),
                    }),
                })),
            }),
            'semantic_frame': non_empty_dict(),
            'frame_actions': non_empty_dict(),
            'analytics_info': non_empty_dict(),
        }))

        check_analytics_info(r, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': has_entries({
                    'source': 'music',
                }),
            }), has_entries({
                'timestamp': '1579488271002000',
                'music_event': has_entries({
                    'answer_type': 'Filters',
                }),
            })),
            'product_scenario_name': 'music',
        }))

        music_sdk_uri = get_response(r).Layout.Directives[0].OpenUriDirective.Uri
        uri_info = urlparse(music_sdk_uri)
        assert uri_info.scheme == 'musicsdk'
        assert not uri_info.path

        query_info = parse_qs(uri_info.query)
        query_info.pop('aliceSessionId', None)  # not needed anymore
        assert query_info == {
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-radio'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'radio': [radio_type],
            'tag': [radio_tag],
        }

        return str(r)  # for debug purposes

    def test_search_text_blocks_radio(self, alice):
        '''
        Наличие слота search_text должно заблокировать включение радио
        P.S. А в колонках логика посложнее, там может ответить либо радио, либо что-то еще
        '''
        r = alice(voice('включи веселый квин'))
        check_response(r, texts=['Включаю: Queen.'], output_speeches=['Включаю'])
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-artist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'artist': ['79215'],  # artist_id of "Queen"
        })
        return str(r)


@pytest.mark.skip
class TestsNavigatorRadioBass(_TestsNavigatorRadioBase):
    pass


class TestsNavigatorRadioHw(_TestsNavigatorRadioBase):
    pass


# ONDEMAND
class _TestsNavigatorOndemandBase(_TestsNavigatorBase):

    def _check_analytics_info(self, response):
        check_analytics_info(response, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_web_document_event': non_empty_dict(),
            }), has_entries({
                'timestamp': '1579488271002000',
                'request_source_event': non_empty_dict(),
            }), has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': non_empty_dict(),
            }), has_entries({
                'timestamp': '1579488271002000',
                'music_event': non_empty_dict(),
            })),
            'product_scenario_name': 'music',
        }))

    def test_track(self, alice):
        r = alice(voice('включи alter mann'))

        check_response(r, texts=['Включаю: Rammstein, альбом "Sehnsucht", песня "Alter Mann".'], output_speeches=['Включаю'])
        check_suggests_common(r)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-track'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'track': ['22769'],  # track_id of "Alter Mann"
        })
        self._check_analytics_info(r)

        return str(r)  # for debug purposes

    @pytest.mark.skip
    def test_album_from_multiple_artists(self, alice):
        '''
        Поле "is_various":"true" в ответе web_answer значит, что альбом - сборник (т.е. авторов несколько)
        Тогда в саджестах нужно показать топ-3 артиста от жанра, к которому принадлежит альбом
        '''
        r = alice(voice('включи альбом industrial metal'))

        check_is_various(r, is_various=True)
        check_response(r, texts=['Включаю: сборник, альбом "Industrial Metal".'], output_speeches=['Включаю'])
        check_suggests_common(r)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-album'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'album': ['143374'],  # album_id of "Industrial Metal"
        })
        self._check_analytics_info(r)

        # проверка что в саджестах НЕ альбомы
        suggests = get_response(r).Layout.SuggestButtons
        assert len(suggests) == 4
        for i in range(3):
            assert 'альбом' not in suggests[i].ActionButton.Title.lower()

        return str(r)  # for debug purposes

    def test_album_from_single_artist(self, alice):
        '''
        Поле "is_various":"false" в ответе web_answer значит, что автор у альбома только один
        Тогда в саджестах нужно показать три альбома от того же артиста
        '''
        r = alice(voice('включи альбом mutter'))

        check_is_various(r, is_various=False)
        check_response(r, texts=['Включаю: Rammstein, альбом "Mutter".'], output_speeches=['Включаю'])
        check_suggests_common(r)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-album'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'album': ['3542'],  # album_id of "Mutter"
        })
        self._check_analytics_info(r)

        suggests = get_response(r).Layout.SuggestButtons
        assert len(suggests) == 4
        for i in range(3):
            assert re.match(r'Rammstein, альбом ".*"', suggests[i].ActionButton.Title)

        return str(r)  # for debug purposes

    def test_artist(self, alice):
        r = alice(voice('включи rammstein'))

        check_response(r, texts=['Включаю: Rammstein.'], output_speeches=['Включаю'])
        check_suggests_common(r)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-artist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'artist': ['13002'],  # artist_id of "Rammstein"
        })
        self._check_analytics_info(r)

        return str(r)  # for debug purposes

    def test_from_search_track(self, alice):
        '''
        Некоторые песни могут прийти из поиска, например если сказать не название песни,
        а строчки из неё
        '''
        r = alice(voice('алиса давай споем песню нисколько мы с тобой не постарели'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == 'Включаю'


@pytest.mark.skip
class TestsNavigatorOndemandBass(_TestsNavigatorOndemandBase):
    pass


class TestsNavigatorOndemandHw(_TestsNavigatorOndemandBase):
    pass


# SHUFFLE FOR ALL TYPES
class _TestsNavigatorShuffleBase(_TestsNavigatorBase):

    def test_ondemand(self, alice):
        r = alice(voice('поставь metallica вперемешку'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-artist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['true'],  # the most important part of the test
            'artist': ['3121'],  # artist_id of "Metallica"
        })
        return str(r)

    def test_radio(self, alice):
        r = alice(voice('поставь рэп вперемешку'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-radio'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['true'],  # the most important part of the test
            'radio': ['genre'],
            'tag': ['rap'],
        })
        return str(r)

    def test_playlist(self, alice):
        r = alice(voice('включи вперемешку плейлист рок'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['true'],  # the most important part of the test
            'kind': ['1001'],
            'owner': ['414787002'],
        })
        return str(r)

    def test_special_playlist(self, alice):
        r = alice(voice('включи вперемешку плейлист дня'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['true'],  # the most important part of the test
            'kind': ['127167070'],
            'owner': ['503646255'],
        })
        return str(r)


@pytest.mark.skip
class TestsNavigatorShuffleBass(_TestsNavigatorShuffleBase):
    pass


class TestsNavigatorShuffleHw(_TestsNavigatorShuffleBase):
    pass


# REPEAT FOR ALL TYPES
class _TestsNavigatorRepeatBase(_TestsNavigatorBase):

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_ondemand(self, alice):
        r = alice(voice('поставь на повторе du hast'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-track'],
            'play': ['true'],
            'repeat': ['repeatAll'],  # the most important part of the test
            'shuffle': ['false'],
            'track': ['22771'],  # track_id of "Du Hast"
        })
        return str(r)

    def test_radio(self, alice):
        r = alice(voice('поставь на повторе рэп'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-radio'],
            'play': ['true'],
            'repeat': ['repeatAll'],  # the most important part of the test
            'shuffle': ['false'],
            'radio': ['genre'],
            'tag': ['rap'],
        })
        return str(r)

    def test_playlist(self, alice):
        r = alice(voice('поставь на повторе плейлист рок'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatAll'],  # the most important part of the test
            'shuffle': ['false'],
            'kind': ['1001'],
            'owner': ['414787002'],
        })
        return str(r)

    def test_special_playlist(self, alice):
        r = alice(voice('поставь на повторе плейлист дня'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatAll'],  # the most important part of the test
            'shuffle': ['false'],
            'kind': ['127167070'],
            'owner': ['503646255'],
        })
        return str(r)


@pytest.mark.skip
class TestsNavigatorRepeatBass(_TestsNavigatorRepeatBase):
    pass


class TestsNavigatorRepeatHw(_TestsNavigatorRepeatBase):
    pass


# MUSIC FLOW
@pytest.mark.oauth(auth.Amediateka)
class _TestsNavigatorFlowBase(_TestsNavigatorUnauthorizedBase):

    def test_smoke(self, alice):
        '''
        Тег должен быть "onyourwave". Другие теги устарели и будут заменены.
        У пользователя auth.Amediateka нормальный тег, а у auth.YandexPlus еще нет.
        '''
        r = alice(voice('включи музыку'))
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-radio'],
            'play': ['true'],
            'radio': ['user'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'tag': ['onyourwave'],
        })
        check_analytics_info(r, has_entries({
            'intent': 'personal_assistant.scenarios.music_play',
            'events': contains(has_entries({
                'timestamp': '1579488271002000',
                'selected_source_event': has_entries({
                    'source': 'music',
                }),
            }), has_entries({
                'timestamp': '1579488271002000',
                'music_event': has_entries({
                    'answer_type': 'Filters',
                }),
            })),
            'product_scenario_name': 'music',
        }))
        return str(r)


@pytest.mark.skip
class TestsNavigatorFlowBass(_TestsNavigatorFlowBase):
    pass


class TestsNavigatorFlowHw(_TestsNavigatorFlowBase):
    pass


# MISCELLANEOUS TESTS OUTSIDE OF GROUPS
# ALL OF THEM FALL BACK TO OLD CODE
class _TestsNavigatorMiscBase(_TestsNavigatorBase):

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_special_playlist_is_album(self, alice):
        r = alice(voice('поставь твои новогодние песни'))

        check_response(r, texts=['Включаю: Алиса, альбом "YANY".'], output_speeches=['Включаю'])
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-album'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'album': ['4924870'],  # album_id of Yet Another New Year
        })

        return str(r)

    def test_fixlist(self, alice):
        r = alice(voice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу'))

        text = 'Включаю трек по запросу: Дора, Не исправлюсь.'
        check_response(r, texts=[text], output_speeches=[text])
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_yandexnavi-alice-track'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'track': ['67996858'],  # track_id of "Дора, Не исправлюсь"
        })

        return str(r)


@pytest.mark.experiments('hw_music_sdk_client_disable_navigator')
class TestsNavigatorMiscBass(_TestsNavigatorMiscBase):
    pass


class TestsNavigatorMiscHw(_TestsNavigatorMiscBase):
    pass
