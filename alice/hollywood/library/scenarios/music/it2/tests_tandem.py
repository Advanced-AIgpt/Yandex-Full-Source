import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice


logger = logging.getLogger(__name__)

EXPERIMENTS = [
    'hw_music_thin_client',
]

ENV_STATE_SIMPLE = {
    "devices": [
        {
            "application": {
                "app_id": "com.yandex.tv.alice",
                "app_version": "1.1000.1000",
                "device_id": "FOLLOWER_DEVICE_ID",
                "device_model": "yandexmodule_2",
                "uuid": "secret_uuid_1",
            },
            "device_state": {
                "subscription_state": {
                    "subscription": "none",
                },
                "tandem_state": {
                    "connected": True,
                },
            },
            "supported_features": [],
        },
        {
            "application": {
                "app_id": "ru.yandex.quasar.app",
                "app_version": "1.0",
                "device_id": "LEADER_DEVICE_ID",
                "device_model": "Station_2",
                "uuid": "secret_uuid_2",
            },
            "device_state": {
                "subscription_state": {
                    "subscription": "none",
                },
                "tandem_state": {
                    "connected": True,
                },
            },
            "supported_features": [],
        },
    ],
    "groups": [
        {
            "devices": [
                {
                    "id": "FOLLOWER_DEVICE_ID",
                    "platform": "yandexmodule_2",
                    "role": "follower",
                },
                {
                    "id": "LEADER_DEVICE_ID",
                    "platform": "yandexstation_2",
                    "role": "leader",
                },
            ],
            "type": "tandem"
        },
    ],
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class _TestsTandemBase:
    pass


class TestsTandem(_TestsTandemBase):

    @pytest.mark.device_state(device_id='FOLLOWER_DEVICE_ID')
    @pytest.mark.environment_state(ENV_STATE_SIMPLE)
    def test_simple(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Музыку я играю в колонке. Попросите меня, пожалуйста, включить ее там.',
            'Все музыкальные заказы я исполняю на колонке. Попросите меня об этом там, пожалуйста.',
        ]
