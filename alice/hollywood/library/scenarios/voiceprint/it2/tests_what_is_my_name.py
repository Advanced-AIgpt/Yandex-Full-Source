import pytest

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, Biometry, ClientBiometry
from alice.hollywood.library.scenarios.voiceprint.proto.voiceprint_pb2 import TVoiceprintState, TVoiceprintEnrollState  # noqa
from alice.hollywood.library.scenarios.voiceprint.it2.voiceprint_helpers import (
    BIO_CAPABILITY_ENV_STATE,
    EXPERIMENTS,
    EXPERIMENTS_WHAT_IS_MY_NAME,
    DEFAULT_GUEST_USER,
)

WHAT_IS_MY_NAME_FRAME = 'personal_assistant.scenarios.what_is_my_name'


def _make_client_biometry(matched_user=DEFAULT_GUEST_USER, is_owner_enrolled=True):
    return ClientBiometry(matched_user, is_owner_enrolled)


# this user has uid 1035351314 just like default uid in Biometry:
# https://a.yandex-team.ru/arcadia/alice/library/python/testing/megamind_request/input_dialog.py?rev=r9353182#L82
@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='Voiceprint', handle='voiceprint')
@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_WHAT_IS_MY_NAME)
class TestsBase:
    pass


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsWhatIsMyNameServerBiometry(TestsBase):

    def test_what_is_my_name_known_user(self, alice):
        r = alice(voice('как меня зовут', biometry=Biometry(is_known=True, known_user_name='Боб')))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == WHAT_IS_MY_NAME_FRAME
        assert 'Боб' in r.run_response.ResponseBody.Layout.OutputSpeech

    def test_what_is_my_name_unknown_user(self, alice):
        r = alice(voice('как меня зовут', biometry=Biometry(is_known=False, known_user_name='Боб')))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == WHAT_IS_MY_NAME_FRAME
        assert 'Боб' not in r.run_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
class TestsWhatIsMyNameClientBiometry(TestsBase):

    def test_what_is_my_name_known_owner(self, alice):
        r = alice(voice('как меня зовут', biometry=Biometry(is_known=True), client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == WHAT_IS_MY_NAME_FRAME
        assert 'Боб' in r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='no_owner'),
    ])
    def test_what_is_my_name_known_guest(self, alice, is_owner_enrolled):
        old_user_name = 'саша'
        pers_id = 'PersId-123890'

        r = alice(voice('как меня зовут', client_biometry=ClientBiometry(
            DEFAULT_GUEST_USER, is_owner_enrolled,
            personality_user_name=old_user_name,
            pers_id=pers_id,
        )))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == WHAT_IS_MY_NAME_FRAME
        assert 'Саша' in r.run_response.ResponseBody.Layout.OutputSpeech
