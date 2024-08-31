from urllib.parse import unquote

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


class TestPalmOpenSitesAndApps(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-23
        https://testpalm.yandex-team.ru/testcase/alice-1074
        https://testpalm.yandex-team.ru/testcase/alice-1087
        https://testpalm.yandex-team.ru/testcase/alice-1449
        https://testpalm.yandex-team.ru/testcase/alice-1511
        https://testpalm.yandex-team.ru/testcase/alice-2237
    """

    owners = ('sparkle',)

    @pytest.mark.parametrize('surface', [surface.watch])
    @pytest.mark.parametrize('command', [
        'открой сайт авито',
        'открой сайт газеты ру',
        'открой приложение инстаграм',
        'открой контакты',
    ])
    def test_elari_watch(self, alice, command):
        response = alice(command)
        assert response.intent == intent.ProhibitionError
        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
    ])
    @pytest.mark.parametrize('command, host', [
        ('открой сайт госуслуг', 'm.gosuslugi.ru'),
        ('перейди на сайт центробанка', 'cbr.ru'),
        ('покажи сайт авто ру', 'm.auto.ru'),
    ])
    def test_open_sites(self, alice, command, host):
        response = alice(command)
        assert response.intent == intent.OpenSiteOrApp

        # check voice response
        assert response.has_voice_response()
        assert response.output_speech_text == 'Открываю'

        # check card
        assert response.text_card
        assert response.button('Открыть')

        # check directive
        assert response.directive.name == directives.names.OpenUriDirective
        assert host in response.directive.payload.uri

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
    ])
    def test_open_which_site(self, alice):
        response = alice('открой сайт')
        assert response.intent == intent.OpenSiteOrApp
        assert response.text == 'Какой сайт вы хотите открыть?'
        assert not response.directive

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
    ])
    @pytest.mark.parametrize('command, package, url', [
        # ('открой одноклассники', 'ru.ok.android', 'https://m.ok.ru/'),  - постоянно флапает
        # ('открой вконтакте', 'com.vkontakte.android', 'https://m.vk.com'),  - постоянно флапает
        ('открой яндекс карты', 'ru.yandex.yandexmaps', 'https://mobile.yandex.ru/maps'),
    ])
    def test_open_app(self, alice, command, package, url):
        response = alice(command)
        assert response.intent == intent.OpenSiteOrApp

        # check voice response
        assert response.has_voice_response()
        assert response.output_speech_text == 'Открываю'

        # check card
        assert response.text_card
        assert response.button('Открыть')

        # check directive
        assert response.directive.name == directives.names.OpenUriDirective
        uri = unquote(response.directive.payload.uri)
        assert f'package={package}' in uri and f'browser_fallback_url={url}' in uri

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [
        # surface.smart_tv,  # waiting for ALICEINFRA-353
        surface.station,
    ])
    @pytest.mark.parametrize('command', [
        'открой хром',
        'запусти яндекс маркет',
        'зайди в вк',
    ])
    def test_open_app_unsupported(self, alice, command):
        response = alice(command)
        assert response.intent == intent.OpenSiteOrApp

        # check voice response
        assert response.has_voice_response()
        assert 'не могу' in response.output_speech_text

    @pytest.mark.experiments('search_filter_set')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_change_search_modes(self, alice):
        alice.application.AppVersion = '8.70'

        response = alice('включи поиск без ограничений')
        assert response.intent == intent.SearchFilterSetNoFilter
        assert alice.filtration_level == 0

        response = alice('включи семейный поиск')
        assert response.intent == intent.SearchFilterSetFamily
        assert alice.filtration_level == 2

        response = alice('отключи семейный поиск')
        assert response.intent == intent.SearchFilterReset
        assert alice.filtration_level == 1

        response = alice('включи умеренный поиск')
        assert response.intent == intent.SearchFilterReset
        assert alice.filtration_level == 1

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEASSESSORS-207')
    @pytest.mark.parametrize('surface', [surface.navi])
    @pytest.mark.parametrize('command, url', [
        # alice-1511
        ('открой сайт гибдд', 'http://www.gibdd.ru/'),
        ('перейди на сайт центробанка', 'https://www.cbr.ru/'),
        ('покажи сайт авто ру', 'https://m.auto.ru/'),

        # alice-1449
        ('открой браузер', 'http://'),
        ('открой одноклассники', 'vins://open_url_with_fallback?url=intent%3A%2F%2F%23Intent%3Bpackage%3Dru.ok.android%3BS.browser_fallback_url%3Dhttps%253A%252F%252Fm.ok.ru%252F%3Bend'),
        ('открой вконтакте', 'vins://open_url_with_fallback?url=intent%3A%2F%2F%23Intent%3Bpackage%3Dcom.vkontakte.android%3BS.browser_fallback_url%3Dhttps%253A%252F%252Fm.vk.com%252F%3Bend'),
    ])
    def test_navi(self, alice, command, url):
        response = alice(command)
        assert response.intent == intent.OpenSiteOrApp
        assert response.text == 'Я, конечно, могу это сделать, но рекомендую вам не отвлекаться от дороги.'

        assert len(response.suggests) == 1
        suggest = response.suggest('Открыть')
        assert suggest

        # корректность нажатия на саджест
        assert len(suggest.directives) == 2
        directive = suggest.directives[0]
        assert directive.name == directives.names.OpenUriDirective
        assert directive.payload.uri == url

        # корректность открытия голосом
        response = alice('открыть')
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == url
        assert not response.text  # это причина xfail
