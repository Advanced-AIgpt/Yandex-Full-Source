import base64

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice, text, server_action
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint


saas_stubber = create_stubber_fixture(
    'saas-searchproxy-prestable.yandex.net',
    17000,
    [
        StubberEndpoint('/', ['GET']),
    ],
    stubs_subdir='saas_stubs'
)


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['tv_channels']


@pytest.fixture(scope="function")
def srcrwr_params(saas_stubber):
    return {
        'TV_CHANNELS_PROXY': f'localhost:{saas_stubber.port}',
    }


def bg_disable_form(form_name):
    config = f"""
        Language: "ru"
        Frames: [
            {{
                Name: "{form_name}"
            }}
        ]
    """
    return base64.b64encode(config.encode()).decode()

# эта конструкция выключет на стороне бегемота одну из форм
DISABLE_V1_FORM = bg_disable_form('alice.switch_tv_channel')
DISABLE_V2_TEXT_FORM = bg_disable_form('alice.switch_tv_channel2_text')
DISABLE_V2_NUM_FORM = bg_disable_form('alice.switch_tv_channel2_num')

NO_SUCH_CHANNEL_RESPONSES = [
    "Упс. Такого канала нет.",
    "Простите, не нахожу этот канал.",
    "Кажется, такого канала нет.",
    "Этого нет. Может, другой посмотрим?",
]


@pytest.mark.scenario(name='TvChannels', handle='tv_channels')
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.experiments(f'bg_frame_aggregator_config_patch_base64={DISABLE_V2_TEXT_FORM}',
                         f'bg_frame_aggregator_config_patch_base64={DISABLE_V2_NUM_FORM}',
                          'bg_fresh_granet_prefix=alice.switch_tv')
class TestBackComp:
    """
    В этих кейсах мы проверяем обратную совместимость с формой v1
    """

    def test_open_channel_by_name(self, alice):
        """
        Включаем по названию существующий канал
        """
        r = alice(voice('включи канал звезда'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            "live-tv://android.media.tv/channel/vh/100405?device_id=ffffffff"

    def test_open_channel_by_number(self, alice):
        """
        Включаем по номеру
        """
        r = alice(voice('включи канал 1050'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            "live-tv://android.media.tv/channel/vh/100743?device_id=ffffffff"

    def test_nonexisting_channel(self, alice):
        """
        Несуществующий канал выдаст Fail
        """
        r = alice(text('включи канал хххх'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Cards[0].Text in NO_SUCH_CHANNEL_RESPONSES


@pytest.mark.scenario(name='TvChannels', handle='tv_channels')
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.experiments(f'bg_frame_aggregator_config_patch_base64={DISABLE_V1_FORM}',
                          'bg_fresh_granet_prefix=alice.switch_tv',
                          'tv_channels_form_v2_enabled')
class TestTvChannels2:
    def test_open_channel_by_name(self, alice):
        """
        Включаем по названию существующий канал
        """
        r = alice(voice('включи канал звезда'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            "live-tv://android.media.tv/channel/vh/100405?device_id=ffffffff"

    def test_open_channel_by_number(self, alice):
        """
        По номеру
        """
        r = alice(voice('включи канал 1050'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            "live-tv://android.media.tv/channel/vh/100743?device_id=ffffffff"

    def test_open_channel_by_number_or_title(self, alice):
        """
        Когда матчится и номер и название канала, то приоритет отдается номеру
        """
        r = alice(voice('включи первый канал'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            "live-tv://android.media.tv/channel/vh/101371?device_id=ffffffff"

    def test_open_unknown_existing_channel(self, alice):
        """
        Неизвестный канал, который существует в базе
        Попадает в слот unknownChannel и находится в saas - значит включается
        """
        r = alice(voice('включи канал авто плюс hd'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            "live-tv://android.media.tv/channel/vh/100153?device_id=ffffffff"

    def test_open_youtube_channel(self, alice):
        """
        Неизвестный канал, которого в базе нет
        Попадает в слот unkownChannel и не находится в saas - поэтому будет irrelevant
        """
        r = alice(voice('включи канал вилсаком'))
        assert r.is_run_irrelevant()

    def test_nenexistant_channel_by_number(self, alice):
        """
        Канал с несуществующим номером дает ответ Relevant - Not found
        """
        r = alice(voice('включи канал 44'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Cards[0].Text in NO_SUCH_CHANNEL_RESPONSES


@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.scenario(name='TvChannels', handle='tv_channels')
class TestNextPrevChannel:
    """
    Предыдущий/следующий канал
    """
    def build_server_action(self, uri):
        payload = {
            "typed_semantic_frame": {
                "switch_tv_channel_semantic_frame": {
                    "uri": {
                        "string_value": uri,
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "switch_tv_channel",
            }
        }

        return server_action(name='@@mm_semantic_frame', payload=payload)

    def test_next_channel(self, alice):
        uri = "content://android.media.tv/channel/823?input=com"
        sf = self.build_server_action(uri)
        response = alice(sf)
        assert response is not None
        assert response.scenario_stages() == {'run'}
        assert response.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == uri
