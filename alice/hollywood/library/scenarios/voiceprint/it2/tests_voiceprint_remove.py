import pytest
import re

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, Biometry, ClientBiometry
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries
from alice.hollywood.library.scenarios.voiceprint.proto.voiceprint_pb2 import TVoiceprintState, TVoiceprintRemoveState  # noqa
from hamcrest import assert_that, has_entries, matches_regexp, contains_inanyorder, is_not, empty, not_none, contains
from alice.hollywood.library.scenarios.voiceprint.it2.voiceprint_helpers import (
    BIO_CAPABILITY_ENV_STATE,
    EXPERIMENTS,
    EXPERIMENTS_REMOVE,
    ENROLLMENT_MULTIACC_START_WITHOUT_NAME_RESPONSE_RE,
    DEFAULT_GUEST_USER,
    ENROLLMENT_VIA_IOT_IS_NEEDED_RESPONSE_RE,
    assert_successful_enrollment_start,
)


REMOVE_FRAME = 'personal_assistant.scenarios.voiceprint_remove'
REMOVE_FINISH_INTENT = 'personal_assistant.scenarios.voiceprint_remove__finish'
REMOVE_CANCEL_FRAME_EMULATED = 'alice.voiceprint.remove.cancel__emulated'
CONFIRM_FRAME = 'alice.proactivity.confirm'
VOICEPRINT_DECLINE_INTENT = 'alice.voiceprint.decline'


CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES = [
    "Окей, Боб. Если вы и правда хотите, чтобы я перестала вас узнавать — скажите: «Алиса, забудь мой голос». А если передумаете, скажите - «отмена».",
    "Нет проблем, Боб. Если вы действительно хотите, чтобы я перестала вас узнавать — скажите: «Алиса, забудь мой голос». Или - «отмена», если передумаете.",
]


CONFIRM_VOICEPRINT_REMOVE_BY_INCOGNITO_RESPONSES = [
    "Окей. Если вы и правда хотите, чтобы я перестала узнавать голос Боба — скажите: «Алиса, забудь голос». А если передумаете, скажите - «отмена».",
    "Нет проблем. Если вы действительно хотите, чтобы я перестала узнавать голос Боба — скажите: «Алиса, забудь голос». Или - «отмена», если передумаете.",
]


CONFIRM_VOICEPRINT_REMOVE_NO_NAME_RESPONSES = [
    "Окей. Если вы и правда хотите, чтобы я перестала вас узнавать — скажите: «Алиса, забудь мой голос». А если передумаете, скажите - «отмена».",
    "Нет проблем. Если вы действительно хотите, чтобы я перестала вас узнавать — скажите: «Алиса, забудь мой голос». Или - «отмена», если передумаете.",
]


CAN_NOT_REMOVE_UNKNOWN_USER_VOICEPRINT_RESPONSES = [
    "Мне не знаком ваш голос.",
    "Не узна+ю ваш голос.",
    "Напрягла память, но не смогла вспомнить ваш голос.",
    "Я очень старалась, но ваш голос вспомнить не смогла.",
    "Я не узна+ю вас, человек.",
    "Извините, но я вас не знаю.",
    "Кажется, мы не знакомы.",
]


CANCEL_REMOVE_RESPONSES = [
    "Хорошо-хорошо. Ничего не трогаем.",
    "Окей, оставляем как есть.",
    "Ладно, не буду удалять.",
]


TOO_MANY_ENROLLED_USERS_RESPONSES = [
    "Пока я умею запоминать только один голос на устройстве. Увы.",
    "Пока я умею запоминать только один голос на устройстве. Простите.",
]


def _assert_voiceprint_remove_run_response(r, intent, output_speech_responses=None):
    assert r.scenario_stages() == {'run'}
    assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
    assert r.run_response.ResponseBody.AnalyticsInfo.Intent == intent
    if output_speech_responses:
        assert r.run_response.ResponseBody.Layout.OutputSpeech in output_speech_responses


def _make_client_biometry(matched_user=DEFAULT_GUEST_USER, is_owner_enrolled=True):
    return ClientBiometry(matched_user, is_owner_enrolled)


# this user has uid 1035351314 just like default uid in Biometry:
# https://a.yandex-team.ru/arcadia/alice/library/python/testing/megamind_request/input_dialog.py?rev=r9353182#L82
@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='Voiceprint', handle='voiceprint')
@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_REMOVE)
class TestBase:
    pass


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsVoiceprintRemoveServerBiometry(TestBase):

    def test_voiceprint_remove(self, alice):
        r = alice(voice('забудь мой голос', biometry=Biometry(is_known=True)))
        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('забудь мой голос', biometry=Biometry(is_known=True)))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == REMOVE_FINISH_INTENT
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': matches_regexp(r'.*познакомиться снова.*Просто скажите:.*'),
                    'directives': contains(
                        has_only_entries({
                            'remove_voiceprint_directive': has_only_entries({
                                'user_id': '1035351314',
                            }),
                        }),
                    ),
                })
            })
        }))

    def test_voiceprint_remove_by_unknown_user(self, alice):
        r = alice(voice('забудь голос', biometry=Biometry(is_known=False)))
        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=CONFIRM_VOICEPRINT_REMOVE_BY_INCOGNITO_RESPONSES)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('забудь голос', biometry=Biometry(is_known=False)))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == REMOVE_FINISH_INTENT
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': matches_regexp(r'.*познакомиться снова.*Просто скажите:.*'),
                    'directives': contains(
                        has_only_entries({
                            'remove_voiceprint_directive': has_only_entries({
                                'user_id': '1035351314',
                            }),
                        }),
                    ),
                })
            })
        }))

    def test_voiceprint_remove_no_biometry(self, alice):
        r = alice(voice('забудь мой голос'))
        _assert_voiceprint_remove_run_response(r, REMOVE_FINISH_INTENT, output_speech_responses=CAN_NOT_REMOVE_UNKNOWN_USER_VOICEPRINT_RESPONSES)
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

    @pytest.mark.parametrize('is_known, confirm_responses', [
        pytest.param(True, CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES, id='owner'),
        pytest.param(False, CONFIRM_VOICEPRINT_REMOVE_BY_INCOGNITO_RESPONSES, id='incognito'),
    ])
    def test_voiceprint_remove_cancel(self, alice, is_known, confirm_responses):
        r = alice(voice('забудь голос', biometry=Biometry(is_known=is_known)))
        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=confirm_responses)

        r = alice(voice('отмена', biometry=Biometry(is_known=is_known)))
        _assert_voiceprint_remove_run_response(r, REMOVE_CANCEL_FRAME_EMULATED, output_speech_responses=CANCEL_REMOVE_RESPONSES)


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
class TestsVoiceprintRemoveClientBiometry(TestBase):

    def test_voiceprint_remove_owner(self, alice):
        r = alice(voice('забудь мой голос',
                        client_biometry=_make_client_biometry(),
                        biometry=Biometry(is_known=True)))

        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('забудь мой голос',
                        client_biometry=_make_client_biometry(),
                        biometry=Biometry(is_known=True)))

        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == REMOVE_FINISH_INTENT
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': matches_regexp(r'.*познакомиться снова.*Просто скажите:.*'),
                    'directives': contains_inanyorder(
                        has_only_entries({
                            'remove_voiceprint_directive': has_only_entries({
                                'user_id': '1035351314',
                                'pers_id': is_not(empty()),
                            }),
                        }),
                        has_only_entries({
                            'multiaccount_remove_account_directive': has_only_entries({
                                'puid': '1035351314',
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        }),
                    ),
                })
            })
        }))

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.experiments('hw_voiceprint_enable_multiaccount')
    def test_voiceprint_remove_guest_no_owner(self, alice):
        client_biometry = _make_client_biometry(matched_user=DEFAULT_GUEST_USER, is_owner_enrolled=False)

        r = alice(voice('забудь мой голос', client_biometry=client_biometry))
        # TODO(klim-roma): change output_speech_responses when guest's username can be fetched
        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=CONFIRM_VOICEPRINT_REMOVE_NO_NAME_RESPONSES)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('забудь мой голос', client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == REMOVE_FINISH_INTENT
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': "Готово, выкинула из головы. Но если что — мы всегда можем снова подружиться в приложении «Дом с Алисой».",
                    'directives': contains_inanyorder(
                        has_only_entries({
                            'remove_voiceprint_directive': has_only_entries({
                                'user_id': '1035351314',
                                'pers_id': is_not(empty()),
                            }),
                        }),
                        has_only_entries({
                            'multiaccount_remove_account_directive': has_only_entries({
                                'puid': '1035351314',
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        }),
                    ),
                })
            })
        }))

    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='unknown_owner'),
    ])
    @pytest.mark.experiments('hw_voiceprint_enable_multiaccount')
    def test_voiceprint_remove_unknown_user(self, alice, is_owner_enrolled):
        biometry = Biometry(is_known=False) if is_owner_enrolled else None

        r = alice(voice('забудь мой голос',
                        client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=is_owner_enrolled),
                        biometry=biometry))

        _assert_voiceprint_remove_run_response(r, REMOVE_FINISH_INTENT)
        assert r.run_response.ResponseBody.Layout.OutputSpeech.endswith('Хотите, чтобы я узнавала вас по голосу?')
        assert r.run_response.ResponseBody.Layout.ShouldListen
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

    @pytest.mark.parametrize('matched_user, confirm_responses', [
        pytest.param(DEFAULT_GUEST_USER, CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES, id='owner'),
    ])
    def test_voiceprint_remove_cancel(self, alice, matched_user, confirm_responses):
        r = alice(voice('забудь голос',
                        client_biometry=_make_client_biometry(matched_user=matched_user),
                        biometry=Biometry(is_known=matched_user is not None)))

        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=confirm_responses)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('отмена',
                        client_biometry=_make_client_biometry(matched_user=matched_user),
                        biometry=Biometry(is_known=matched_user is not None)))

        _assert_voiceprint_remove_run_response(r, REMOVE_CANCEL_FRAME_EMULATED, output_speech_responses=CANCEL_REMOVE_RESPONSES)


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
@pytest.mark.experiments('hw_voiceprint_enable_multiaccount')
class TestsVoiceprintClientBiometrySuggestEnrollemt(TestBase):

    def test_voiceprint_accept_suggest_no_owner(self, alice):
        r = alice(voice('забудь мой голос',
                        client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=False)))

        _assert_voiceprint_remove_run_response(r, REMOVE_FINISH_INTENT)
        assert r.run_response.ResponseBody.Layout.OutputSpeech.endswith('Хотите, чтобы я узнавала вас по голосу?')
        assert r.run_response.ResponseBody.Layout.ShouldListen
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

        r = alice(voice('да',
                        client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=False)))

        assert_successful_enrollment_start(r, CONFIRM_FRAME, output_speech_re=ENROLLMENT_MULTIACC_START_WITHOUT_NAME_RESPONSE_RE)

    @pytest.mark.parametrize('owner_gender', [
        pytest.param('male', id='male_owner'),
        pytest.param('female', id='female_owner'),
    ])
    def test_voiceprint_accept_suggest_owner_enrolled(self, alice, owner_gender):
        owner_name = 'саша'
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=True)
        biometry_for_personal_data = Biometry(is_known=False, known_user_name=owner_name, known_user_gender=owner_gender)

        r = alice(voice('забудь мой голос',
                        client_biometry=client_biometry,
                        biometry=biometry_for_personal_data))

        _assert_voiceprint_remove_run_response(r, REMOVE_FINISH_INTENT)
        assert r.run_response.ResponseBody.Layout.OutputSpeech.endswith('Хотите, чтобы я узнавала вас по голосу?')
        assert r.run_response.ResponseBody.Layout.ShouldListen
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

        r = alice(voice('хочу',
                        client_biometry=client_biometry,
                        biometry=biometry_for_personal_data))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == CONFIRM_FRAME

        match = re.match(ENROLLMENT_VIA_IOT_IS_NEEDED_RESPONSE_RE, r.run_response.ResponseBody.Layout.OutputSpeech)
        assert match, "IOT enrollment should be suggested"
        assert match.group(1) == match.group(3), "Owner's name is expected to be matched twice"
        assert match.group(1).lower() == owner_name
        assert match.group(2) == 'должен' if owner_gender == 'male' else match.group(2) == 'должна'

        # TODO(klim-roma): TSendPushDirective should be sent
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

    def test_voiceprint_decline_suggest_no_owner(self, alice):
        r = alice(voice('забудь мой голос',
                        client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=False),
                        biometry=Biometry(is_known=False)))

        _assert_voiceprint_remove_run_response(r, REMOVE_FINISH_INTENT)
        assert r.run_response.ResponseBody.Layout.OutputSpeech.endswith('Хотите, чтобы я узнавала вас по голосу?')
        assert r.run_response.ResponseBody.Layout.ShouldListen
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

        r = alice(voice('нет',
                        client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=False),
                        biometry=Biometry(is_known=False)))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == VOICEPRINT_DECLINE_INTENT
        assert not r.run_response.ResponseBody.Layout.OutputSpeech
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
@pytest.mark.experiments('hw_voiceprint_enable_fallback_to_server_biometry')
class TestsVoiceprintClientBiometryFallbackToServerBiometry(TestBase):

    @pytest.mark.experiments('hw_voiceprint_enable_multiaccount')
    def test_voiceprint_remove_no_match_multiacc_enabled(self, alice):
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=True)

        r = alice(voice('забудь мой голос',
                        client_biometry=client_biometry,
                        biometry=Biometry(is_known=True)))

        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('забудь мой голос',
                        client_biometry=client_biometry,
                        biometry=Biometry(is_known=True)))

        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == REMOVE_FINISH_INTENT
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': "Готово, выкинула из головы. Но если что — мы всегда можем снова подружиться в приложении «Дом с Алисой».",
                    'directives': contains_inanyorder(
                        has_only_entries({
                            'remove_voiceprint_directive': has_entries({
                                'user_id': '1035351314',
                            }),
                        }),
                        has_only_entries({
                            'multiaccount_remove_account_directive': has_only_entries({
                                'puid': '1035351314',
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        }),
                    ),
                })
            })
        }))

    def test_voiceprint_remove_no_match(self, alice):
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=True)

        r = alice(voice('забудь мой голос',
                        client_biometry=client_biometry,
                        biometry=Biometry(is_known=True)))

        _assert_voiceprint_remove_run_response(r, REMOVE_FRAME, output_speech_responses=CONFIRM_VOICEPRINT_REMOVE_KNOWN_RESPONSES)
        assert r.run_response.ResponseBody.Layout.ShouldListen

        r = alice(voice('забудь мой голос',
                        client_biometry=client_biometry,
                        biometry=Biometry(is_known=True)))

        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == REMOVE_FINISH_INTENT
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': matches_regexp(r'.*познакомиться снова.*Просто скажите:.*'),
                    'directives': contains_inanyorder(
                        has_only_entries({
                            'remove_voiceprint_directive': has_entries({
                                'user_id': '1035351314',
                            }),
                        }),
                        has_only_entries({
                            'multiaccount_remove_account_directive': has_only_entries({
                                'puid': '1035351314',
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        }),
                    ),
                })
            })
        }))
