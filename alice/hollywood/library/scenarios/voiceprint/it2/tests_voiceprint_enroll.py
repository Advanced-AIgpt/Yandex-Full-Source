import pytest

from alice.hollywood.library.python.testing.it2 import auth, surface, region
from alice.hollywood.library.python.testing.it2.input import voice, server_action, BiometryScoringProvider, ClientBiometry
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries
from alice.hollywood.library.scenarios.voiceprint.proto.voiceprint_pb2 import TVoiceprintState, TVoiceprintEnrollState  # noqa
from hamcrest import assert_that, has_entries, matches_regexp, is_not, empty, contains, contains_inanyorder, not_none
from alice.hollywood.library.scenarios.voiceprint.it2.voiceprint_helpers import (
    BIO_CAPABILITY_ENV_STATE,
    EXPERIMENTS,
    EXPERIMENTS_SET_MY_NAME,
    ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE,
    ENROLLMENT_MULTIACC_START_WITHOUT_NAME_RESPONSE_RE,
    DEFAULT_GUEST_USER,
    SWEAR_USER_NAME_RESPONSES,
    assert_successful_enrollment_stage,
    assert_successful_enrollment_start,
)


ENROLL_PHRASES = [
    'включи музыку',
    'поставь будильник на семь часов',
    'какие сейчас пробки?',
    'давай сыграем в города',
    'поставь звуки природы',
]


ENROLL_FRAME = 'personal_assistant.scenarios.voiceprint_enroll'
ENROLL_COLLECT_FRAME = 'personal_assistant.scenarios.voiceprint_enroll__collect_voice'
ENROLL_FINISH_FRAME = 'personal_assistant.scenarios.voiceprint_enroll__finish'
ENROLL_GUEST_FRAME = 'alice.guest.enrollment.start'


INVALID_REGION_RESPONSES = [
    "Сценарий «Запомни мой голос» не доступен в вашем регионе.",
    "Вы мне нравитесь, но я пока не смогу запомнить ваше имя в этом регионе.",
]

ENROLLMENT_MULTIACC_WAIT_READY_RESPONSE_RE = r'(.+), если имя верное, скажите: «я готов» или «я готова»\. '\
                                             r'Если вы хотите, чтобы я обращалась к вам по-другому, скажите «Меня зовут Аристарх», только вместо «Аристарх» должно быть ваше имя\.'

ENROLLMENT_MULTIACC_START_COLLECT_RESPONSE_RE = r'Отлично! Сейчас я попрошу вас повторить 5 фраз\. Постарайтесь говорить как обычно, будто общаетесь с другом\. '\
                                                r'Лучше, чтобы на фоне ничего не шумело\. Если что-то будет не понятно, скажите: «Алиса, повтори»\.'

ENROLLMENT_MULTIACC_FINISH_RESPONSE_RE = r'Мне очень приятно познакомиться с вами, (.+)\. Теперь я буду понимать, когда со мной говорите именно вы\. И включать именно вашу музыку\. '\
                                         r'Надеюсь, этого не случится, но если вы захотите прервать нашу дружбу, скажите: «(.+) забудь мой голос»\.'


def _make_client_biometry(matched_user=DEFAULT_GUEST_USER, is_owner_enrolled=True):
    return ClientBiometry(matched_user, is_owner_enrolled)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.scenario(name='Voiceprint', handle='voiceprint')
@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_SET_MY_NAME)
class TestBase:
    pass


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsEnrollmentStart(TestBase):

    @pytest.mark.region(region.Madrid)
    def test_server_biometry_invalid_region_fail(self, alice):
        r = alice(voice('давай познакомимся'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FRAME
        assert r.run_response.ResponseBody.Layout.OutputSpeech in INVALID_REGION_RESPONSES
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

    @pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
    @pytest.mark.region(region.Madrid)
    @pytest.mark.parametrize('command, intent, output_speech_re', [
        pytest.param('давай познакомимся', ENROLL_FRAME, ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE, id='nameless'),
        pytest.param('меня зовут Вася', ENROLL_COLLECT_FRAME, r'.*(скажите «я готов» или «я готова» — и мы продолжим знакомиться).*', id='with_name'),
    ])
    def test_client_biometry_invalid_region_success(self, alice, command, intent, output_speech_re):
        r = alice(voice(command, client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=False)))
        assert_successful_enrollment_start(r, intent, output_speech_re=output_speech_re)

    @pytest.mark.parametrize('command, intent, output_speech_re', [
        pytest.param('давай познакомимся', ENROLL_FRAME, ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE, id='nameless'),
        pytest.param('меня зовут Вася', ENROLL_COLLECT_FRAME, r'.*(скажите «я готов» или «я готова» — и мы продолжим знакомиться).*', id='with_name'),
    ])
    def test_server_biometry_enrollment_start(self, alice, command, intent, output_speech_re):
        r = alice(voice(command))
        assert_successful_enrollment_start(r, intent, output_speech_re=output_speech_re)

    @pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
    @pytest.mark.parametrize('command, intent, output_speech_re', [
        pytest.param('давай познакомимся', ENROLL_FRAME, ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE, id='nameless'),
        pytest.param('меня зовут Вася', ENROLL_COLLECT_FRAME, r'.*(скажите «я готов» или «я готова» — и мы продолжим знакомиться).*', id='with_name'),
    ])
    def test_client_biometry_enrollment_start(self, alice, command, intent, output_speech_re):
        r = alice(voice(command, client_biometry=_make_client_biometry(matched_user=None, is_owner_enrolled=False)))
        assert_successful_enrollment_start(r, intent, output_speech_re=output_speech_re)

    def test_server_biometry_swear_user_name_fail(self, alice):
        r = alice(voice('называй меня хуй'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
        assert r.run_response.ResponseBody.Layout.OutputSpeech in SWEAR_USER_NAME_RESPONSES
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.parametrize('ready_command, gender', [
    pytest.param('я готов', 'male', id='male'),
    pytest.param('я готова', 'female', id='female'),
])
class TestsEnrollmentFullFlow(TestBase):

    def test_server_biometry_enrollment(self, alice, ready_command, gender):
        r = alice(voice('давай познакомимся'))
        assert_successful_enrollment_start(r, ENROLL_FRAME, output_speech_re=ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE)

        r = alice(voice('меня зовут саша'))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME,
                                            output_speech_re=r'.*скажите «я готов» или «я готова» — и мы продолжим знакомиться.*')

        r = alice(voice(ready_command))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME, output_speech_re=r'.*Ура! Я попрошу вас повторить 5 фраз.*')

        for i in range(4):
            r = alice(voice(ENROLL_PHRASES[i], biometry=BiometryScoringProvider()))
            assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME)

        r = alice(voice(ENROLL_PHRASES[-1], biometry=BiometryScoringProvider()))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'should_listen': True,
                    'output_speech': matches_regexp(r'.*Саша.*'),
                    'directives': contains(
                        has_only_entries({
                            'save_voiceprint_directive': has_entries({
                                'user_id': is_not(empty()),
                                'requests': is_not(empty())
                            }),
                        }),
                    ),
                }),
                'ServerDirectives': contains_inanyorder(
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/user_name',
                            'value': 'саша',
                        }),
                    }),
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/guest_uid',
                            'value': '1234567890',
                        }),
                    }),
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/gender',
                            'value': gender,
                        }),
                    })
                ),
            }),
        }))

        # check that we don't do anything illegal if client sends alice.guest.finish.enrollment TSF for some reason
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_finish_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_irrelevant()

    @pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
    @pytest.mark.experiments('hw_enrollment_directives')
    def test_client_biometry_first_owner_enrollment(self, alice, ready_command, gender):
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=False)

        r = alice(voice('давай познакомимся', client_biometry=client_biometry))
        assert_successful_enrollment_start(r, ENROLL_FRAME, output_speech_re=ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE)

        r = alice(voice('меня зовут саша', client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME,
                                            output_speech_re=r'.*скажите «я готов» или «я готова» — и мы продолжим знакомиться.*')

        r = alice(voice(ready_command, client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME, output_speech_re=r'.*Ура! Я попрошу вас повторить 5 фраз.*')
        directives = r.run_response.ResponseBody.Layout.Directives
        assert directives[0].HasField('EnrollmentStartDirective')
        assert len(directives[0].EnrollmentStartDirective.PersId) > 0
        assert directives[0].EnrollmentStartDirective.TimeoutMs > 0
        assert directives[0].EnrollmentStartDirective.Puid == 1083813279
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        for i in range(4):
            r = alice(voice(ENROLL_PHRASES[i], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
            assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME)

        r = alice(voice(ENROLL_PHRASES[-1], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'should_listen': True,
                    'output_speech': matches_regexp(r'.*Саша.*'),
                    'directives': contains(
                        has_only_entries({
                            'save_voiceprint_directive': has_entries({
                                'user_id': is_not(empty()),
                                'requests': is_not(empty())
                            })
                        }),
                        has_only_entries({
                            'enrollment_finish_directive': has_only_entries({
                                'pers_id': is_not(empty())
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        })
                    ),
                }),
                'ServerDirectives': contains_inanyorder(
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/user_name',
                            'value': 'саша',
                        }),
                    }),
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/guest_uid',
                            'value': '1234567890',
                        }),
                    }),
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/gender',
                            'value': gender,
                        }),
                    })
                ),
            }),
        }))

        # check that we don't do anything illegal if client sends alice.guest.finish.enrollment TSF for some reason
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_finish_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_irrelevant()

    @pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='no_owner'),
    ])
    @pytest.mark.experiments('hw_enrollment_directives')
    def test_client_biometry_guest_enrollment(self, alice, ready_command, gender, is_owner_enrolled):
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=is_owner_enrolled)

        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_start_semantic_frame': {
                    'puid': {
                        'string_value': '1083955728',
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert_successful_enrollment_start(r, ENROLL_GUEST_FRAME, output_speech_re=ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE)

        r = alice(voice('меня зовут саша', client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME,
                                            output_speech_re=r'.*скажите «я готов» или «я готова» — и мы продолжим знакомиться.*')

        r = alice(voice(ready_command, client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME, output_speech_re=r'.*Ура! Я попрошу вас повторить 5 фраз.*')
        directives = r.run_response.ResponseBody.Layout.Directives
        assert directives[0].HasField('EnrollmentStartDirective')
        assert len(directives[0].EnrollmentStartDirective.PersId) > 0
        assert directives[0].EnrollmentStartDirective.TimeoutMs > 0
        assert directives[0].EnrollmentStartDirective.Puid == 1083955728
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        for i in range(4):
            r = alice(voice(ENROLL_PHRASES[i], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
            assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME)

        r = alice(voice(ENROLL_PHRASES[-1], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert not r.apply_response.ResponseBody.Layout.OutputSpeech
        assert not r.apply_response.ResponseBody.ServerDirectives
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'directives': contains(
                        has_only_entries({
                            'enrollment_finish_directive': has_only_entries({
                                'pers_id': is_not(empty()),
                                'send_guest_enrollment_finish_frame': True,
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        })
                    ),
                }),
            }),
        }))
        assert_that(r.apply_response_pyobj, is_not(has_entries({
            'response': has_entries({
                'ServerDirectives': contains(
                    has_entries({
                        'UpdateDatasyncDirective': not_none()
                    }),
                ),
            }),
        })))

        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_finish_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'should_listen': True,
                    'output_speech': matches_regexp(r'.*Саша.*'),
                }),
                'ServerDirectives': contains_inanyorder(
                    has_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': matches_regexp(r'/v1/personality/profile/alisa/kv/enrollment__PersId-.+__user_name'),
                            'value': 'саша',
                        }),
                        'meta': has_entries({
                            'apply_for': 'CurrentUser',
                        }),
                    }),
                    has_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': matches_regexp(r'/v1/personality/profile/alisa/kv/enrollment__PersId-.+__gender'),
                            'value': gender,
                        }),
                        'meta': has_entries({
                            'apply_for': 'CurrentUser',
                        }),
                    })
                ),
            }),
        }))
        assert_that(r.apply_response_pyobj, is_not(has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'directives': contains(
                        has_only_entries({
                            'enrollment_finish_directive': not_none(),
                        }),
                    ),
                }),
            }),
        })))

    @pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='no_owner'),
    ])
    @pytest.mark.experiments('hw_enrollment_directives', 'hw_voiceprint_enable_multiaccount')
    def test_client_biometry_guest_multiacc_phrases(self, alice, ready_command, gender, is_owner_enrolled):
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=is_owner_enrolled)

        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_start_semantic_frame': {
                    'puid': {
                        'string_value': '1083955728',
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert_successful_enrollment_start(r, ENROLL_GUEST_FRAME, output_speech_re=ENROLLMENT_MULTIACC_START_WITHOUT_NAME_RESPONSE_RE)

        r = alice(voice('меня зовут марина', client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME,
                                            output_speech_re=ENROLLMENT_MULTIACC_WAIT_READY_RESPONSE_RE)

        r = alice(voice(ready_command, client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME, output_speech_re=ENROLLMENT_MULTIACC_START_COLLECT_RESPONSE_RE)
        directives = r.run_response.ResponseBody.Layout.Directives
        assert directives[0].HasField('EnrollmentStartDirective')
        assert len(directives[0].EnrollmentStartDirective.PersId) > 0
        assert directives[0].EnrollmentStartDirective.TimeoutMs > 0
        assert directives[0].EnrollmentStartDirective.Puid == 1083955728
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        for i in range(4):
            r = alice(voice(ENROLL_PHRASES[i], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
            assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME)

        r = alice(voice(ENROLL_PHRASES[-1], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert not r.apply_response.ResponseBody.Layout.OutputSpeech
        assert not r.apply_response.ResponseBody.ServerDirectives
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'directives': contains(
                        has_only_entries({
                            'enrollment_finish_directive': has_only_entries({
                                'pers_id': is_not(empty()),
                                'send_guest_enrollment_finish_frame': True,
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        })
                    ),
                }),
            }),
        }))
        assert_that(r.apply_response_pyobj, is_not(has_entries({
            'response': has_entries({
                'ServerDirectives': contains(
                    has_entries({
                        'UpdateDatasyncDirective': not_none()
                    }),
                ),
            }),
        })))

        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_finish_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': matches_regexp(ENROLLMENT_MULTIACC_FINISH_RESPONSE_RE),
                }),
                'ServerDirectives': contains_inanyorder(
                    has_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': matches_regexp(r'/v1/personality/profile/alisa/kv/enrollment__PersId-.+__user_name'),
                            'value': 'марина',
                        }),
                        'meta': has_entries({
                            'apply_for': 'CurrentUser',
                        }),
                    }),
                    has_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': matches_regexp(r'/v1/personality/profile/alisa/kv/enrollment__PersId-.+__gender'),
                            'value': gender,
                        }),
                        'meta': has_entries({
                            'apply_for': 'CurrentUser',
                        }),
                    })
                ),
            }),
        }))
        assert_that(r.apply_response_pyobj, is_not(has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'directives': contains(
                        has_only_entries({
                            'enrollment_finish_directive': not_none(),
                        }),
                    ),
                }),
            }),
        })))
        assert_that(r.apply_response_pyobj, is_not(has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'should_listen': True,
                }),
            }),
        })))

    @pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
    @pytest.mark.experiments('hw_enrollment_directives', 'hw_voiceprint_enable_multiaccount')
    def test_first_owner_enrollment_multiacc_phrases(self, alice, ready_command, gender):
        client_biometry = _make_client_biometry(matched_user=None, is_owner_enrolled=False)

        r = alice(voice('давай познакомимся', client_biometry=client_biometry))
        assert_successful_enrollment_start(r, ENROLL_FRAME, output_speech_re=ENROLLMENT_MULTIACC_START_WITHOUT_NAME_RESPONSE_RE)

        r = alice(voice('меня зовут марина', client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME,
                                            output_speech_re=ENROLLMENT_MULTIACC_WAIT_READY_RESPONSE_RE)

        r = alice(voice(ready_command, client_biometry=client_biometry))
        assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME, output_speech_re=ENROLLMENT_MULTIACC_START_COLLECT_RESPONSE_RE)
        directives = r.run_response.ResponseBody.Layout.Directives
        assert directives[0].HasField('EnrollmentStartDirective')
        assert len(directives[0].EnrollmentStartDirective.PersId) > 0
        assert directives[0].EnrollmentStartDirective.TimeoutMs > 0
        assert directives[0].EnrollmentStartDirective.Puid == 1083813279
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        for i in range(4):
            r = alice(voice(ENROLL_PHRASES[i], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
            assert_successful_enrollment_stage(r, ENROLL_COLLECT_FRAME)

        r = alice(voice(ENROLL_PHRASES[-1], biometry=BiometryScoringProvider(), client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.apply_response.ResponseBody.AnalyticsInfo.Intent == ENROLL_FINISH_FRAME
        assert_that(r.apply_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'output_speech': matches_regexp(ENROLLMENT_MULTIACC_FINISH_RESPONSE_RE),
                    'directives': contains(
                        has_only_entries({
                            'save_voiceprint_directive': has_entries({
                                'user_id': is_not(empty()),
                                'requests': is_not(empty())
                            })
                        }),
                        has_only_entries({
                            'enrollment_finish_directive': has_only_entries({
                                'pers_id': is_not(empty())
                            }),
                        }),
                        has_only_entries({
                            'tts_play_placeholder': not_none(),
                        })
                    ),
                }),
                'ServerDirectives': contains_inanyorder(
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/user_name',
                            'value': 'марина',
                        }),
                    }),
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/guest_uid',
                            'value': '1234567890',
                        }),
                    }),
                    has_only_entries({
                        'UpdateDatasyncDirective': has_entries({
                            'key': '/v1/personality/profile/alisa/kv/gender',
                            'value': gender,
                        }),
                    })
                ),
            }),
        }))
        assert_that(r.apply_response_pyobj, is_not(has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'should_listen': True,
                }),
            }),
        })))

        # check that we don't do anything illegal if client sends alice.guest.finish.enrollment TSF for some reason
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'guest_enrollment_finish_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test'
            }
        }))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_irrelevant()
