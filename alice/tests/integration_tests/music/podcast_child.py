import json
import re

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


DEFAULT_PODCAST_URI = 'https://music.yandex.ru/users/414787002/playlists/1104/?from=alice&mob=0'
CHILD_DEFAULT_PODCAST_URI = 'https://music.yandex.ru/users/970829816/playlists/1064/?from=alice&mob=0'
PODCAST_ZUBOCHISTKI_URI = 'https://music.yandex.ru/album/10768374/?from=alice&mob=0'
PODCAST_KAK_ZHIT_URI = 'https://music.yandex.ru/album/6271118/?from=alice&mob=0'


def _assert_podcast_response(response, uri):
    assert response.scenario in (scenario.Vins, scenario.HollywoodMusic)
    assert response.intent in (intent.MusicPodcast, intent.MusicPlay)
    assert response.directive.name == directives.names.AudioPlayDirective
    if response.scenario == scenario.Vins:
        assert 'answer' in response.slots
        assert json.loads(response.slots['answer'].string)['uri'] == uri
    else:
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert music_event.uri == uri


def _assert_podcast_response_forbidden_in_children_mode(response):
    assert response.text in [
        'Я не могу поставить эту музыку в детском режиме.',
        'Знаю такое, но не могу поставить в детском режиме.',
        'В детском режиме такое включить не получится.',
        'Не могу. Знаете почему? У вас включён детский режим.',
        'Я бы и рада, но у вас включён детский режим поиска.',
        'Не выйдет. У вас включён детский режим, а это не для детских ушей.',
    ]
    assert not response.directive


def _assert_podcast_response_forbidden_in_safe_mode(response):
    assert response.text == 'Лучше всего послушать этот подкаст вместе с родителями.'
    assert not response.directive


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
class _TestPodcastChild(object):
    owners = ('karina-usm',)


@pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.children})
class TestPodcastChildrenMode(_TestPodcastChild):

    def test_children_default_podcast(self, alice):
        response = alice('включи подкаст')
        assert 'Включаю' in response.text
        assert 'Подкасты: топ-100' in response.text
        _assert_podcast_response(response, DEFAULT_PODCAST_URI)

    def test_forbidden_in_children_mode(self, alice):
        response = alice('давай послушаем подкаст истории русского секса история про сашу')
        _assert_podcast_response_forbidden_in_children_mode(response)

    def test_allowed_in_children_mode(self, alice):
        response = alice('включи подкаст как жить')
        assert re.match(r'(Включаю|Продолжаю) подкаст "Как жить".', response.text)
        _assert_podcast_response(response, PODCAST_KAK_ZHIT_URI)


@pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
class TestPodcastSafeMode(_TestPodcastChild):

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-14414')
    def test_safe_default_podcast(self, alice):
        response = alice('включи подкаст')
        assert response.text.startswith('Включаю "Подкасты для детей".')
        _assert_podcast_response(response, CHILD_DEFAULT_PODCAST_URI)

    @pytest.mark.version(hollywood=150)
    def test_forbidden_in_safe_mode(self, alice):
        response = alice('включи подкаст как жить')
        _assert_podcast_response_forbidden_in_safe_mode(response)

    def test_allowed_in_safe_mode(self, alice):
        response = alice('включи подкаст зубочистки')
        assert re.match(r'(Включаю|Продолжаю) подкаст "Зубочистки".', response.text)
        _assert_podcast_response(response, PODCAST_ZUBOCHISTKI_URI)
