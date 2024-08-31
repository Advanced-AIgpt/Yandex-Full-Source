import re

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.automotive, surface.uma])
class TestPalmAutoFmRadio(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1391
    """

    owners = ('kupriyanov-m', 'g:maps-auto-crossplatform')

    @pytest.mark.parametrize('command', ['Включи фм радио'])
    def test_radio_onboarding(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexauto://media_control?source=fm'

        assert response.text == 'Включаю радио.'
        assert response.text == response.output_speech_text

    @pytest.mark.region(region.StPetersburg)
    @pytest.mark.parametrize('command', ['Включи радио Сибирь'])
    def test_radio_not_found_in_region(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert not response.directive

        assert response.text in [
            'Эта станция недоступна в вашем регионе. Такие дела.',
            'К сожалению, в вашем регионе эта станция недоступна.',
        ]
        assert response.text == response.output_speech_text

    @pytest.mark.parametrize('command, radio_name', [
        # just some popular stations
        pytest.param(
            'Включи радио юмор-фм',
            'юмор fm',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи радио монте-карло',
            'радио монте-карло',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи фм радио джаз',
            'jazz',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи авторадио',
            'авторадио',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи радио книга',
            'радио книга',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи кэпитал фм',
            'capital fm',
            marks=pytest.mark.region(region.Moscow),
        ),
        # different pronounces
        pytest.param(
            'Включи радио европа плюс',
            'европа плюс',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи европу плюс',
            'европа плюс',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'вруби радио европу плюс',
            'европа плюс',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Алиса включи пожалуйста радиостанцию европа плюс',
            'европа плюс',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Давай послушаем радио европа плюс',
            'европа плюс',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи питер фм',
            'питер fm',
            marks=pytest.mark.region(region.StPetersburg),
        ),
        # frequency
        pytest.param(
            'Включи 100 и 5 фм',
            'жара fm',
            marks=pytest.mark.region(region.Moscow),
        ),
        pytest.param(
            'Включи девяносто четыре и восемь фм',
            'говорит москва',
            marks=pytest.mark.region(region.Moscow),
        ),
    ])
    def test_radio_search(self, alice, command, radio_name):
        response = alice(command)
        assert response.scenario == scenario.AutomotiveRadio
        assert response.intent == intent.ProtocolRadioPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert re.match('yandexauto://fm_radio\\?frequency=\\d+&name=.+', response.directive.payload.uri), \
               f'Got wrong URI: "{response.directive.payload.uri}"'

        assert re.match(f'^включаю \"{radio_name}\"$', response.text, re.IGNORECASE), \
               f'Got response: "{response.text}", expected: "{radio_name}"'
        assert re.match(f'^включаю {radio_name}$', response.output_speech_text, re.IGNORECASE), \
               f'Got response: "{response.output_speech_text}", expected: "{radio_name}"'


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.old_automotive])
class TestPalmOldAutoFmRadio(object):
    """
    Частично https://testpalm.yandex-team.ru/testcase/alice-1800
    """

    owners = ('kupriyanov-m', 'g:maps-auto-crossplatform')

    @pytest.mark.parametrize('command', ['Включи фм радио', 'Включи радио'])
    def test_radio_onboarding(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.application == 'car'
        assert response.directive.payload.intent == 'media_select'
        assert response.directive.payload.params.radio == 'Монте-Карло'

        assert response.text == 'Включаю радио.'
        assert response.text == response.output_speech_text

    @pytest.mark.region(region.Moscow)
    @pytest.mark.parametrize('command, radio_name, frequency', [
        # just some popular stations
        ('Включи радио юмор-фм', 'юмор fm', None),
        ('Включи радио монте-карло', 'монте-карло', None),
        ('Включи авторадио', 'авторадио', None),
        # frequency
        ('Включи девяносто четыре и восемь фм', 'говорит москва', '94.8'),
    ])
    def test_radio_search(self, alice, command, radio_name, frequency):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.application == 'car'
        assert response.directive.payload.intent == 'media_select'
        assert response.directive.payload.params.radio.lower() == radio_name

        radio = f'частоту "{frequency}"' if frequency else f'"{radio_name}"'
        assert re.match(f'^включаю {radio}.$', response.text, re.IGNORECASE), \
               f'Got response: "{response.text}", expected: "{radio}"'
        assert response.text == response.output_speech_text
