import pytest
import re

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.hamcrest import non_empty_dict
from alice.hollywood.library.python.testing.it2.input import voice, Biometry, ClientBiometry
from alice.hollywood.library.scenarios.voiceprint.proto.voiceprint_pb2 import TVoiceprintState, TVoiceprintEnrollState  # noqa
from hamcrest import assert_that, has_entries, contains, is_not
from alice.hollywood.library.scenarios.voiceprint.it2.voiceprint_helpers import (
    BIO_CAPABILITY_ENV_STATE,
    EXPERIMENTS,
    EXPERIMENTS_SET_MY_NAME,
    EXPLAIN_ENROLLMENT_RESPONSE_RE,
    SWEAR_USER_NAME_RESPONSES,
    DEFAULT_GUEST_USER,
    assert_successful_enrollment_start,
    assert_successful_enrollment_stage,
)

ENROLL_FINISH_INTENT = 'personal_assistant.scenarios.voiceprint_enroll__finish'
SET_MY_NAME_FRAME = 'personal_assistant.scenarios.set_my_name'

CAN_NOT_SET_NAME_FOR_UNKNOWN_USER_RESPONSE_RE = r'Я могу изменить имя пользователя, голос которого я уже знаю, а ваш голос мне не знак.*'

EXPLAIN_SET_MY_NAME_RESPONSE_RE = r'.*Если хотите изменить имя, скажите:.*'


def _make_client_biometry(matched_user=DEFAULT_GUEST_USER, is_owner_enrolled=True):
    return ClientBiometry(matched_user, is_owner_enrolled)


# this user has uid 1035351314 just like default uid in Biometry:
# https://a.yandex-team.ru/arcadia/alice/library/python/testing/megamind_request/input_dialog.py?rev=r9353182#L82
@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='Voiceprint', handle='voiceprint')
@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_SET_MY_NAME)
class TestsBase:
    pass


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsSetMyNameServerBiometry(TestsBase):

    def test_set_my_name_known_user(self, alice):
        r = alice(voice('называй меня ваня', biometry=Biometry(is_known=True)))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == SET_MY_NAME_FRAME
        assert 'Ваня' in r.apply_response.ResponseBody.Layout.OutputSpeech
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': 'Хорошо, Боб. Теперь я буду называть вас Ваня.',
                }),
                'ServerDirectives': contains(has_entries({
                    'UpdateDatasyncDirective': has_entries({
                        'key': '/v1/personality/profile/alisa/kv/user_name',
                        'value': 'ваня',
                    }),
                    'meta': has_entries({
                        'apply_for': 'DeviceOwner',
                    }),
                })),
            }),
        }))

    def test_set_my_name_unknown_user(self, alice):
        r = alice(voice('называй меня ваня', biometry=Biometry(is_known=False)))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == SET_MY_NAME_FRAME
        assert re.match(CAN_NOT_SET_NAME_FOR_UNKNOWN_USER_RESPONSE_RE, r.run_response.ResponseBody.Layout.OutputSpeech)
        assert_that(r.run_response_pyobj, has_entries({
            'response_body': is_not(has_entries({
                'ServerDirectives': contains(has_entries({
                    'UpdateDatasyncDirective': non_empty_dict(),
                })),
            })),
        }))

    @pytest.mark.parametrize('is_known, output_speech_re', [
        pytest.param(True, EXPLAIN_SET_MY_NAME_RESPONSE_RE, id='owner'),
        pytest.param(False, CAN_NOT_SET_NAME_FOR_UNKNOWN_USER_RESPONSE_RE, id='incognito'),
    ])
    def test_set_my_name_no_username_slot(self, alice, is_known, output_speech_re):
        r = alice(voice('алиса измени мое имя', biometry=Biometry(is_known=is_known)))
        assert_successful_enrollment_stage(r, SET_MY_NAME_FRAME, output_speech_re=output_speech_re)

    def test_set_my_name_no_username_slot_no_biometry(self, alice):
        r = alice(voice('алиса измени мое имя'))
        assert_successful_enrollment_start(r, SET_MY_NAME_FRAME, output_speech_re=EXPLAIN_ENROLLMENT_RESPONSE_RE)

    def test_set_my_name_swear(self, alice):
        r = alice(voice('измени мое имя на хуй', biometry=Biometry(is_known=True)))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.Layout.OutputSpeech in SWEAR_USER_NAME_RESPONSES
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
class TestsSetMyNameClientBiometry(TestsBase):

    def test_set_my_name_known_owner(self, alice):
        r = alice(voice('называй меня ваня', biometry=Biometry(is_known=True), client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == SET_MY_NAME_FRAME
        assert 'Ваня' in r.apply_response.ResponseBody.Layout.OutputSpeech
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': 'Хорошо, Боб. Теперь я буду называть вас Ваня.',
                }),
                'ServerDirectives': contains(has_entries({
                    'UpdateDatasyncDirective': has_entries({
                        'key': '/v1/personality/profile/alisa/kv/user_name',
                        'value': 'ваня',
                    }),
                    'meta': has_entries({
                        'apply_for': 'DeviceOwner',
                    }),
                })),
            }),
        }))

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='no_owner'),
    ])
    def test_set_my_name_known_guest(self, alice, is_owner_enrolled):
        old_user_name = 'саша'
        pers_id = 'PersId-123890'

        r = alice(voice('называй меня ваня', client_biometry=ClientBiometry(
            DEFAULT_GUEST_USER, is_owner_enrolled,
            personality_user_name=old_user_name,
            pers_id=pers_id,
        )))

        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == SET_MY_NAME_FRAME
        assert 'Ваня' in r.apply_response.ResponseBody.Layout.OutputSpeech
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': 'Хорошо, Саша. Теперь я буду называть вас Ваня.',
                }),
                'ServerDirectives': contains(has_entries({
                    'UpdateDatasyncDirective': has_entries({
                        'key': f'/v1/personality/profile/alisa/kv/enrollment__{pers_id}__user_name',
                        'value': 'ваня',
                    }),
                    'meta': has_entries({
                        'apply_for': 'CurrentUser',
                    }),
                })),
            }),
        }))
