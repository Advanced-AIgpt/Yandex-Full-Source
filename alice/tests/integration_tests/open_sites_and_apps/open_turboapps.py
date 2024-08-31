import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('enable_turboapps')
class TestOpeningTurboapps(object):

    owners = ('tolyandex', )

    not_supported = [
        'Извините, у меня нет хорошего ответа.',
        'У меня нет ответа на такой запрос.',
        'Я пока не умею отвечать на такие запросы.',
        'Простите, я не знаю что ответить.',
        'Я не могу на это ответить.',
        'Я не могу открывать сайты и приложения на этом устройстве.',
        'Простите, я не могу это сделать на вашем устройстве.',
        'Я бы с радостью, но на этом устройстве не могу, увы.',
        'Очень хочу, но не могу: это устройство не поддерживает подобную функцию.',
    ]

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_station(self, alice):
        response = alice('открой приложение маркета')
        assert response.intent == intent.OpenSiteOrApp
        assert response.text in self.not_supported

        response = alice('включи смешарики')
        assert response.intent == intent.VideoPlay

        response = alice('включи эфир')
        assert response.scenario == scenario.ShowTvChannelsGallery

    @pytest.mark.parametrize('command', [
        'открой приложение маркет',
        'яндекс лавка',
        'открой яндекс лавка',
        'открой иваново текстиль',
        'включи смешарики',
        'вруби яндекс штрафы',
        'складом ру',
    ])
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_searchapp(self, alice, command):
        response = alice(command)
        assert response.intent in [intent.OpenSiteOrApp, intent.Search, intent.Nav]
        if response.intent == intent.Search:
            assert 'Открываю' in response.text
        assert response.directive.name in [directives.names.OpenUriDirective, directives.names.FillCloudUiDirective]
