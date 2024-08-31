import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.Yandex)
class TestPalmFmRadio(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1346
    https://testpalm.yandex-team.ru/testcase/alice-1347
    https://testpalm.yandex-team.ru/testcase/alice-1530
    """

    owners = ('zhigan', )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_radio_1346_onboarding(self, alice):
        response = alice('включи радио')
        assert re.search(r'Включаю|Окей!|Хорошо!', response.text)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('http://music.yandex.ru/fm/')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command_radio, radio_name, radio_path', [
        pytest.param(
            'бизнес фм',
            'Business FM',
            'business_fm',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'русское радио',
            'Русское радио',
            'rusradio',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'монте карло',
            'Монте Карло',
            'monte_carlo',
            marks=[
                pytest.mark.region(region.Moscow),
                pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7257')
            ],
        ),
        pytest.param(
            'радио джаз',
            'Радио JAZZ',
            'jazz',
            marks=[
                pytest.mark.region(region.Moscow),
                pytest.mark.xfail(reason='https://st.yandex-team.ru/MUSICBACKEND-6205')
            ],
        ),
        pytest.param(
            'авторадио',
            'Авторадио',
            'avtoradio',
            marks=pytest.mark.region(region.StPetersburg),
        ),
        pytest.param(
            'маяк',
            'Маяк',
            'mayak',
            marks=[
                pytest.mark.region(region.StPetersburg),
                pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7257')
            ],
        ),
        pytest.param(
            'радио энерджи',
            'Радио Energy',
            'energy',
            marks=pytest.mark.region(region.StPetersburg),
        )
    ])
    def test_radio_1346_1347(self, alice, command_radio, radio_name, radio_path):
        command = f'Включи {command_radio}'
        response = alice(command)

        assert radio_name in response.text
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == f'http://music.yandex.ru/fm/{radio_path}'

        expected_suggests = {'👍', '👎', f'🔍 "{command}"', 'Что ты умеешь?'}
        response_suggests = {suggest.title for suggest in response.suggests}
        assert response_suggests == expected_suggests

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('radio_name, uri', [
        pytest.param(
            'эльдорадио',
            'http://eldoradio.ru',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'москва фм',
            'https://www.mosfm.com',
            marks=pytest.mark.region(region.StPetersburg),
        )
    ])
    def test_alice_1347(self, alice, radio_name, uri):
        command = f'Включи {radio_name}'
        response = alice(command)

        assert re.match('^Я ещё не (поймала эту волну|настроилась на эту (волну|радиостанцию)). Но я могу открыть её сайт.$', response.text)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == uri, response.text

        expected_suggests = {'👍', '👎', f'🔍 "{command}"', 'Что ты умеешь?'}
        response_suggests = {suggest.title for suggest in response.suggests}
        assert response_suggests == expected_suggests

    @pytest.mark.parametrize('surface', surface.smart_speakers)
    @pytest.mark.parametrize('command_radio, text, radio_id', [
        pytest.param(
            'бизнес фм',
            '(Р|р)адио "Business FM"',
            'business_fm',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'русское радио',
            '"Русское радио"',
            'rusradio',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'радио джаз',
            '"Радио JAZZ"',
            'jazz',
            marks=[
                pytest.mark.region(region.Moscow),
                pytest.mark.xfail(reason='https://st.yandex-team.ru/MUSICBACKEND-6205')
            ],
        ),
        pytest.param(
            'хит фм',
            '(Р|р)адио "Хит FM"',
            'hit_fm',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'наше радио',
            '"Наше радио"',
            'nashe',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'радио дача',
            '"Радио Дача"',
            'radio_dacha',
            marks=pytest.mark.region(region.StPetersburg),
        ),
        pytest.param(
            'такси фм',
            '(Р|р)адио "Такси FM"',
            'taxi_fm',
            marks=[
                pytest.mark.region(region.StPetersburg),
                pytest.mark.xfail(reason='https://st.yandex-team.ru/MUSICBACKEND-6205')
            ],
        ),
        pytest.param(
            'радио России',
            '"Радио России"',
            'radio_russia',
            marks=[
                pytest.mark.region(region.StPetersburg),
                pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-795')
            ],
        ),
    ])
    def test_alice_1530(self, alice, command_radio, text, radio_id):
        response = alice(f'Включи {command_radio}')

        assert re.match(f'(Включаю|Окей!|Хорошо!) {text}.', response.text)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay

        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.directive.payload.radioId == radio_id
