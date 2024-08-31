import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest
from library.python import resource


@pytest.mark.parametrize('surface', [surface.station])
class _TestPalmMordovia(object):
    device_state = resource.find('mordovia_video_selection_device_state.json')


class TestPalmMordoviaTvShow(_TestPalmMordovia):
    """
        https://testpalm2.yandex-team.ru/testcase/alice-2281
        https://testpalm2.yandex-team.ru/testcase/alice-2406
    """

    owners = ('akormushkin', )

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_2281(self, alice):
        response = alice('включи номер один')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'kinopoisk'
        assert response.directive.payload.item.episode == 1

        response = alice('следующая серия')
        assert response.scenario == scenario.Vins   # TODO: изменить на Video после схождения VIDEOFUNC-534
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'kinopoisk'
        assert response.directive.payload.item.episode == 2

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_2406(self, alice):
        response = alice('включи номер один')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'kinopoisk'

        response = alice('список сезонов')
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/videoEntity/seasons/?' in response.directive.payload.url

        # Не работает, пока не сделан device_state для СН
        # TODO: реализовать после схождения VIDEOFUNC-885
        # response = alice('следующий сезон')
        # assert response.directive.name == directives.names.MordoviaShowDirective
        # assert '/video/quasar/videoEntity/seasons/?' in response.directive.payload.url
        # assert 'season=2' in response.directive.payload.url
        #
        # response = alice('включи 3 серию')
        # assert response.scenario == scenario.Video
        # assert response.directive.name == directives.names.VideoPlayDirective
        # assert response.directive.payload.item.episode == 3
        # assert response.directive.payload.item.season == 2
        # assert response.directive.payload.item.provider_name == 'kinopoisk'


class TestPalmMordoviaVideo(_TestPalmMordovia):
    """
        https://testpalm2.yandex-team.ru/testcase/alice-2285
    """

    owners = ('akormushkin', )

    def test_alice_2285(self, alice):
        response = alice('включи номер восемь')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'strm'

        response = alice('включи следующее')
        assert response.scenario == scenario.Vins   # TODO: изменить на Video после схождения VIDEOFUNC-534
        assert not response.directive
        assert response.text == 'Для этого видео нет следующего'

        response = alice('назад')
        assert response.scenario == scenario.Vins
        assert response.directive.name == directives.names.GoBackwardDirective
