import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestPalmBluetooth(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-7
    https://testpalm.yandex-team.ru/testcase/alice-1306
    https://testpalm.yandex-team.ru/testcase/alice-1338
    """

    owners = ('zhigan')
    unsupported_nlg = [
        'Я еще не научилась этому. Давно собираюсь, но все времени нет.',
        'Я пока это не умею.',
        'Я еще не умею это.',
        'Я не могу пока, но скоро научусь.',
        'Меня пока не научили этому.',
        'Когда-нибудь я смогу это сделать, но не сейчас.',
        'Надеюсь, я скоро смогу это делать. Но пока нет.',
        'Я не знаю, как это сделать. Извините.',
        'Так делать я еще не умею.',
        'Программист Алексей обещал это вскоре запрограммировать. Но он мне много чего обещал.',
        'К сожалению, этого я пока не умею. Но я быстро учусь.',
    ]
    watch_unsupported_nlg = [
        'Я ещё не разобралась с этим. Но обязательно разберусь.',
        'В часах такое провернуть сложновато.',
        'Я бы и рада, но здесь не могу. Эх.',
        'Здесь точно не получится.',
    ]

    @pytest.mark.parametrize('surface, expected_text', [
        (surface.navi, unsupported_nlg),
        (surface.searchapp, unsupported_nlg),
        (surface.watch, watch_unsupported_nlg),
    ])
    @pytest.mark.parametrize('command', ['Включи', 'Выключи'])
    def test_unsupported(self, alice, command, expected_text):
        response = alice(f'{command} bluetooth')
        assert response.scenario == scenario.Vins
        assert response.intent in [intent.BluetoothOn, intent.BluetoothOff]
        assert not response.directive
        assert response.text in expected_text

    @pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
    def test_bluetooth(self, alice):
        response = alice('Включи bluetooth')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.BluetoothOn
        assert response.directive.name == directives.names.StartBluetoothDirective
        assert response.text in ['Ок', 'Хорошо', 'Сделано', 'Сейчас']

        response = alice('Выруби bluetooth')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.BluetoothOff
        assert response.directive.name == directives.names.StopBluetoothDirective
        assert response.text in ['Ок', 'Хорошо', 'Сделано', 'Сейчас']

    @pytest.mark.parametrize('surface', [surface.dexp])
    def test_install(self, alice):
        response = alice('Включи bluetooth')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.BluetoothOn
        assert response.directive.name == directives.names.StartBluetoothDirective
        assert response.text in [
            'Сначала вам нужно настроить Bluetooth-соединение в приложении Яндекс.',
            'Чтобы настроить соединение с устройством по Bluetooth, откройте приложение Яндекс.',
            'Настроить Bluetooth-соединение с устройством можно в приложении Яндекс.',
        ]
