import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.environment_state({
    'endpoints': [
        {
            'capabilities': [
                {
                    'parameters': {
                        'available_apps': [
                            {
                                'app_id': 'com.685631.3411'
                            },
                            {
                                'app_id': 'youtube.leanback.v4'
                            }
                        ]
                    },
                    'meta': {
                        'supported_directives': [
                            'WebOSLaunchAppDirectiveType',
                            'WebOSShowGalleryDirectiveType'
                        ]
                    },
                    '@type': 'type.googleapis.com/NAlice.TWebOSCapability'
                }
            ],
            'meta': {
                'type': 'WebOsTvEndpointType'
            }
        }
    ]
})
class TestOpenExternalApp(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, app_id, params', [
        pytest.param('открой приложение кинопоиск',
                     'tv.kinopoisk.ru',
                     '{\"pageId\":\"library\"}',
                     id='kinopoisk'),
        pytest.param('открой приложение youtube',
                     'youtube.leanback.v4',
                     '',
                     id='youtube'),
    ])
    def test_open_external_app(self, alice, command, app_id, params):
        response = alice(command)
        assert response.scenario == scenario.SmartDeviceExternalApp
        assert response.text in [
            'Открываю.',
            'Запускаю.',
            'Сделала.',
            'Готово.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == app_id
        assert response.directive.payload.params_json == params

    @pytest.mark.version(hollywood=186)
    def test_try_open_unlisted(self, alice):
        response = alice('открой приложение twitch')
        assert response.scenario == scenario.SmartDeviceExternalApp
        assert response.text in [
            'Упс, это приложение пока недоступно.',
            'Такого приложения у меня пока нет, но я попрошу разработчиков его добавить.',
        ]
