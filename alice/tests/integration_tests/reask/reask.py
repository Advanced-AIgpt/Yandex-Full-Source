import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _assert_reask_response(response):
    assert response.scenario == scenario.Reask
    assert response.text in [
        'Простите, я что-то глуховата. Повторите еще раз.',
        'Повторите, пожалуйста!',
        'Уточните, что именно включить?',
    ]
    assert not response.directive


@pytest.mark.experiments(
    f'mm_enable_protocol_scenario={scenario.Reask}',
    'mm_disable_asr_hypotheses_reranking'
)
class TestReaskBase(object):
    owners = ('ardulat', )


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestReask(TestReaskBase):

    @pytest.mark.voice('positive')
    def test_reask(self, alice):
        response = alice('алиса включи')
        _assert_reask_response(response)

    @pytest.mark.voice('positive')
    def test_already_reasked(self, alice):
        response = alice('алиса включи')
        _assert_reask_response(response)

        response = alice('алиса включи')
        assert response.scenario != scenario.Reask

    @pytest.mark.voice
    @pytest.mark.oauth(auth.YandexPlus)
    def test_player_features(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('пауза')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.ClearQueueDirective

        response = alice('алиса включи', voice='positive')
        assert response.scenario != scenario.Reask


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.navi,
    surface.searchapp,
    surface.smart_tv,
    surface.yabro_win,
    surface.watch,
])
class TestReaskUnsupported(TestReaskBase):

    @pytest.mark.voice('positive')
    def test_reask(self, alice):
        response = alice('алиса включи')
        assert response.scenario != scenario.Reask
