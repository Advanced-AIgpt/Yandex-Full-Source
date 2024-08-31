import re


from alice.hollywood.library.python.testing.it2 import auth


EXPERIMENTS = [
    'hw_voiceprint_enable_bio_capability',
    'hw_voiceprint_update_guest_datasync',
]

EXPERIMENTS_SET_MY_NAME = [
    'bg_beggins_set_my_name',
]

EXPERIMENTS_WHAT_IS_MY_NAME = [
    'bg_beggins_voiceprint_what_is_my_name',
]

EXPERIMENTS_REMOVE = [
    'hw_voiceprint_enable_remove',
    'bg_beggins_voiceprint_remove',
]

TEST_DEVICE_ID = 'feedface-4e95-4fc9-ba19-7bf943a7bf55'

BIO_CAPABILITY_ENV_STATE = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'meta': {
                    },
                    'state': {
                    },
                    '@type': 'type.googleapis.com/NAlice.TBioCapability',
                },
            ],
        },
    ],
}

DEFAULT_GUEST_USER = auth.RobotMultiroom

ENROLLMENT_START_WITHOUT_NAME_RESPONSE_RE = r'.*(Чтобы начать, скажите: «Меня зовут\.\.\.» — и добавьте своё имя).*'

ENROLLMENT_MULTIACC_START_WITHOUT_NAME_RESPONSE_RE = r'Привет! Чтобы начать знакомство, скажите: «Меня зовут Алиса»\. Только вместо «Алиса» должно быть ваше имя\.'

EXPLAIN_ENROLLMENT_RESPONSE_RE = r'.*Это позволит мне понять, когда включить музыку просите именно вы.*'

ENROLLMENT_VIA_IOT_IS_NEEDED_RESPONSE_RE = r'Чтобы я запомнила ваш голос, (.+) (должен|должна) пригласить вас в Дом в приложении «Дом с Алисой»\. '\
                                           r'Отправила пользователю (.+) ссылку на телефон\.'

SWEAR_USER_NAME_RESPONSES = [
    "Могу ошибаться, но на имя это не похоже.",
    "Простите, не расслышала вашего имени.",
]


def assert_successful_enrollment_stage(response, intent, output_speech_re=None):
    assert response.scenario_stages() == {'run'}
    assert response.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'voiceprint'
    assert response.run_response.ResponseBody.AnalyticsInfo.Intent == intent
    if output_speech_re:
        assert re.match(output_speech_re, response.run_response.ResponseBody.Layout.OutputSpeech)


def assert_successful_enrollment_start(response, intent, output_speech_re=None):
    assert_successful_enrollment_stage(response, intent, output_speech_re)
    directives = response.run_response.ResponseBody.Layout.Directives
    assert len(directives) == 1
    assert directives[0].HasField('PlayerPauseDirective')
