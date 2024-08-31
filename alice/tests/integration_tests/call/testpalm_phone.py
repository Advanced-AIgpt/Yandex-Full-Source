import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmPhone(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-8
    """

    owners = ('zhigan',)

    @pytest.mark.parametrize('command', [
        'Позвони маме',
        'Позвони Кате',
    ])
    def test_alice_8(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert not response.intent
        assert response.text in [
            'Я научусь звонить с часов, но пока не умею.',
            'С этим в часах пока не помогу. Но только пока.',
            'Я бы и рада, но ещё не научилась. Всё будет.',
        ]


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.loudspeaker,
    surface.navi,
    surface.station,
    surface.watch,
])
class TestPalmCallStation(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2979
    """

    owners = ('zhigan',)

    @pytest.mark.parametrize('command', [
        'Позвони в колонку',
        'Позвони на колонку',
    ])
    def test_alice_2979(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.DeviceShortcut
        assert response.text == 'К сожалению, не могу открыть страницу устройств Яндекса здесь.'
