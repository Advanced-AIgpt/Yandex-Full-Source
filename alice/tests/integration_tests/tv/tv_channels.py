import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestSelectChannel(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-2870
    https://testpalm.yandex-team.ru/testcase/alice-2871
    '''

    owners = ('akormushkin',)

    @pytest.mark.parametrize('command, uri', [
        ('Включи телеканал муз тв', 'live-tv://android.media.tv/channel/vh/100897'),
        ('Переключи на канал звезда', 'live-tv://android.media.tv/channel/vh/100405'),
        ('Переключи на канал рбк', 'live-tv://android.media.tv/channel/vh/100743'),
    ])
    def test_select_by_name(self, alice, command, uri):
        response = alice(command)
        assert response.scenario == scenario.TvChannels
        assert response.directive.name == directives.names.OpenUriDirective
        assert uri in response.directive.payload.uri

    @pytest.mark.parametrize('command, uri', [
        ('Включи канал номер 1051', 'live-tv://android.media.tv/channel/vh/405'),
    ])
    def test_select_by_number(self, alice, command, uri):
        response = alice(command)
        assert response.scenario == scenario.TvChannels
        assert response.directive.name == directives.names.OpenUriDirective
        assert uri in response.directive.payload.uri

    @pytest.mark.parametrize('command', ['включи канал мой эфир', 'открой мой эфир', 'мой эфир'])
    def test_personal_channel(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.TvChannels}
        assert response.text in [
            'Упс. Такого канала нет.',
            'Простите, не нахожу этот канал.',
            'Кажется, такого канала нет.',
            'Этого нет. Может, другой посмотрим?',
            'К сожалению, не смогла найти этот канал.',
            'Такого канала я не знаю, простите.',
        ]
