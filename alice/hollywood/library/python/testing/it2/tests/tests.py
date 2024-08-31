import json
import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.alice_tests_generator import AliceTestsGenerator
from alice.tests.library.service import AppHost, Hollywood, Megamind
from hamcrest import assert_that, has_entries

logger = logging.getLogger(__name__)


class Servers:

    @property
    def megamind(self):
        return Megamind(port=1234)

    @property
    def scenario_runtime(self):
        return Hollywood(port=3456)

    @property
    def apphost(self):
        return AppHost(port=2345)


@pytest.fixture(scope='function')
def alice(request):
    return AliceTestsGenerator(
        Servers(),
        srcrwr_params={},
        scenario_name='',
        scenario_handle='',
        tests_data_path='',
        test_path='',
        oauth_token='',
        surface_ctor=surface.station,
        application={},
        experiments=[],
        initial_device_state={},
        supported_features=[],
        unsupported_features=[],
        additional_options={},
        iot_user_info='',
        memento='',
        contacts='',
        environment_state='',
        notification_state=None,
        region=None,
        generator_params=[],
)


def test_evo_surface_handles_directives_for_it2(alice):
    sk_response = '''{
    "response": {
        "directives": [{
            "name": "audio_play",
            "payload": {
                "background_mode": "Ducking",
                "callbacks": {
                    "on_failed": {
                        "ignore_answer": true,
                        "is_led_silent": false,
                        "name": "music_thin_client_on_failed",
                        "payload": {
                            "@request_id": "cdb830fa-5dad-43f1-852f-e0b93813fe4d",
                            "@scenario_name": "HollywoodMusic",
                            "events": [{
                                "playAudioEvent": {
                                    "from": "hollywood",
                                    "playId": "JjX1QQ8JtVJ0",
                                    "trackId": "27373919",
                                    "uid": "1083955728"
                                }
                            }]
                        },
                        "type": "server_action"
                    },
                    "on_finished": {
                        "ignore_answer": true,
                        "is_led_silent": false,
                        "name": "music_thin_client_on_finished",
                        "payload": {
                            "@request_id": "cdb830fa-5dad-43f1-852f-e0b93813fe4d",
                            "@scenario_name": "HollywoodMusic",
                            "events": [{
                                "playAudioEvent": {
                                    "from": "hollywood",
                                    "playId": "JjX1QQ8JtVJ0",
                                    "trackId": "27373919",
                                    "uid": "1083955728"
                                }
                            }]
                        },
                        "type": "server_action"
                    },
                    "on_started": {
                        "ignore_answer": true,
                        "is_led_silent": false,
                        "name": "music_thin_client_on_started",
                        "payload": {
                            "@request_id": "cdb830fa-5dad-43f1-852f-e0b93813fe4d",
                            "@scenario_name": "HollywoodMusic",
                            "events": [{
                                "playAudioEvent": {
                                    "from": "hollywood",
                                    "playId": "JjX1QQ8JtVJ0",
                                    "trackId": "27373919",
                                    "uid": "1083955728"
                                }
                            }]
                        },
                        "type": "server_action"
                    },
                    "on_stopped": {
                        "ignore_answer": true,
                        "is_led_silent": false,
                        "name": "music_thin_client_on_stopped",
                        "payload": {
                            "@request_id": "cdb830fa-5dad-43f1-852f-e0b93813fe4d",
                            "@scenario_name": "HollywoodMusic",
                            "events": [{
                                "playAudioEvent": {
                                    "from": "hollywood",
                                    "playId": "JjX1QQ8JtVJ0",
                                    "trackId": "27373919",
                                    "uid": "1083955728"
                                }
                            }]
                        },
                        "type": "server_action"
                    }
                },
                "metadata": {
                    "art_image_url": "avatars.yandex.net/get-music-content/95061/bf438f65.a.3277402-2/%%",
                    "subtitle": "The Beatles",
                    "title": "Yesterday"
                },
                "provider_name": "",
                "scenario_meta": {
                    "@scenario_name": "HollywoodMusic",
                    "owner": "music"
                },
                "screen_type": "Music",
                "stream": {
                    "format": "MP3",
                    "id": "27373919",
                    "offset_ms": 0,
                    "url": "https://s18iva.storage.yandex.net/get-mp3/e2cfccb2a10115221d3cf83d62f6c66d/0005bc2b5891e90e/rmusic/U2FsdGVkX18MhM2SCd3dWtB5R3QnDDZUry5WecAHGTDx4nGcDXoGL3pXCv7hHfSS_dQO9Z3ZBo5jAg4oMnln9ZAhbTUjQQDvtXh4fWJ9Ax0/1869d4d5a5d273b8e022ce958ed06584b1bfe224b077814920ae0b729da9558b/19991?track-id=27373919&from=hollywood&play=false&uid=1083955728"
                },
                "set_pause": false
            },
            "sub_name": "music",
            "type": "client_action"
        }, {
            "is_led_silent": true,
            "name": "@@mm_stack_engine_get_next",
            "payload": {
                "@request_id": "cdb830fa-5dad-43f1-852f-e0b93813fe4d",
                "@scenario_name": "HollywoodMusic",
                "stack_product_scenario_name": "music",
                "stack_session_id": "cdb830fa-5dad-43f1-852f-e0b93813fe4d"
            },
            "type": "server_action"
        }],
        "directives_execution_policy": "BeforeSpeech"
    }
}'''  # noqa
    alice._apply_directives_to_surface(sk_response)

    device_state_json = json.dumps(alice.device_state, indent=4)
    logger.info(f'Actual DeviceState = {device_state_json}')

    assert_that(alice.device_state, has_entries({
        "audio_player": has_entries({
            "player_state": "Playing",
            "current_stream": has_entries({
                "subtitle": "The Beatles",
                "title": "Yesterday"
            })
        }),
        "video": has_entries({
            "current_screen": "music_player"
        }),
        "is_tv_plugged_in": True
    }))
