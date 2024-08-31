import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.experiments(
    f'mm_enable_protocol_scenario={scenario.OnboardingCriticalUpdate}',
    'bg_fresh_granet',
)
@pytest.mark.app(
    device_id='feedface-e8a2-4439-b2e7-000000000001.yandexstation_2',
    timestamp='1618404000',  # fixed client time as recently activated device (created=1618403864)
)
@pytest.mark.device_state(rcu={'is_rcu_connected': True})
@pytest.mark.parametrize('surface', [surface.station_pro])
class TestConfigureSuccess(object):

    owners = ('jan-fazli',)

    def test_response(self, alice):
        response = alice('Приветствие станции')
        assert response.scenario == scenario.OnboardingCriticalUpdate
        assert response.intent == intent.OnboardingConfigureSuccess
        assert response.text == \
            'Поздравляю, ещё одна колонка настроена. У меня есть много фильмов ' \
            'и сериалов специально для вас. Хотите посмотреть новинки?'

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.SuccessStartingOnboardingDirective
        assert response.directives[1].name == directives.names.TtsPlayPlaceholderDirective

        response = alice('Давай')  # frame action
        assert response.scenario == scenario.Video
