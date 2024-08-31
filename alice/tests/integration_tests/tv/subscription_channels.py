import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestSubscriptionChannels(object):

    owners = ('akormushkin',)

    def _has_ya_plus_subscription_channels(self, channel_names):
        return 'tv1000' in channel_names or 'viasat sport' in channel_names or 'da vinci' in channel_names

    def _collect_channel_names(self, response):
        channels = response.directive.payload.items
        return {channel.name.lower().strip() for channel in channels}

    @pytest.mark.oauth(auth.YandexPlus)
    def test_ya_plus_user(self, alice):
        response = alice('что по тв')
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert self._has_ya_plus_subscription_channels(self._collect_channel_names(response))

    @pytest.mark.oauth(auth.Yandex)
    def test_user_without_ya_plus(self, alice):
        response = alice('что по тв')
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert not self._has_ya_plus_subscription_channels(self._collect_channel_names(response))

    @pytest.mark.xfail(reason='known issue, see VIDEOFUNC-702. Flag was not supported')
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('tv_subscription_channels_disabled')
    def test_ya_plus_user_disabled_by_flag(self, alice):
        response = alice('что по тв')
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert not self._has_ya_plus_subscription_channels(self._collect_channel_names(response))

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('channel_name', [
        'viasat sport',
        'viasat nature',
        'national geographic',
        'baby tv',
    ])
    def test_play_supported_subscription_channel(self, alice, channel_name):
        response = alice(f'Включи канал {channel_name}')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream

        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
        assert ''.join(response.directive.payload.item.name.lower().split()) == ''.join(channel_name.lower().split())

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('channel_name', [
        'eurosport',
        'евроспорт',
        pytest.param(
            'fox',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-745')
        ),
    ])
    def test_do_not_play_unsupported_subscription_channel(self, alice, channel_name):
        response = alice(f'Включи канал {channel_name}')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream

        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        assert response.text.startswith('Такого канала нет или он недоступен для вещания в вашем регионе.')
        assert response.text.endswith((
            'Давайте посмотрим что-нибудь ещё.',
            'Но есть много других каналов. Смотрите.'
        ))

        assert channel_name not in self._collect_channel_names(response)
