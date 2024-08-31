import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


SCENARIO_NAME = 'SmartDeviceExternalApp'
SCENARIO_HANDLE = 'smart_device_external_app'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={SCENARIO_NAME}', 'bg_fresh_granet')
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.environment_state({
    "endpoints": [
        {
            "capabilities": [
                {
                    "parameters": {
                        "available_apps": [
                            {
                                "app_id": "com.685631.3411"
                            },
                            {
                                "app_id": "youtube.leanback.v4"
                            }
                        ]
                    },
                    "meta": {
                        "supported_directives": [
                            "WebOSLaunchAppDirectiveType",
                            "WebOSShowGalleryDirectiveType"
                        ]
                    },
                    "@type": "type.googleapis.com/NAlice.TWebOSCapability",
                }
            ],
            "meta": {
                "type": "WebOsTvEndpointType"
            }
        }
    ]
})
class TestLgOpenExternalApp:
    def test_open_kinopoisk(self, alice):
        r = alice(voice('открой приложение кинопоиск'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Открываю.',
            'Запускаю.',
            'Сделала.',
            'Готово.',
        ]
        response_directive = r.run_response.ResponseBody.Layout.Directives[0].WebOSLaunchAppDirective
        assert response_directive.AppId == 'tv.kinopoisk.ru'
        assert response_directive.ParamsJson == b'{\"pageId\":\"library\"}'

        return str(r)

    def test_open_youtube(self, alice):
        r = alice(voice('открой приложение youtube'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Открываю.',
            'Запускаю.',
            'Сделала.',
            'Готово.',
        ]
        response_directive = r.run_response.ResponseBody.Layout.Directives[0].WebOSLaunchAppDirective
        assert response_directive.AppId == 'youtube.leanback.v4'
        assert not response_directive.ParamsJson

        return str(r)

    def test_try_open_unlisted_app(self, alice):
        r = alice(voice('открой приложение twitch'))

        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Упс, это приложение пока недоступно.',
            'Такого приложения у меня пока нет, но я попрошу разработчиков его добавить.',
        ]

        return str(r)
