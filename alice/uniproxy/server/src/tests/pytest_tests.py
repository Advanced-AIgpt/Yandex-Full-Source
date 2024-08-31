import json
import pytest
import os
from tests.uniclient import process_test_session
from alice.uniproxy.library.settings import config

GOOGLE_AUTH_FILE = os.environ.get(
    'GOOGLE_APPLICATION_CREDENTIALS',
    config.get('google', {}).get('asr', {}).get('auth_file', '/opt/google_asr_auth_file'),
)
SKIP_GOOGLE_MARK = pytest.mark.skipif(not os.path.exists(GOOGLE_AUTH_FILE), reason='there is no google auth file')

COMMON_SESSION_FILES = [
    'uniclient_asr_antimat_off.json',
    'uniclient_asr_antimat_on.json',
    'uniclient_asr_antimat_en_off.json',
    'uniclient_asr_antimat_en_on.json',
    'uniclient_asr_capitalize_on.json',
    'uniclient_asr_tr_dialogmapsgpu.json',
    'uniclient_asr_ru_dates.json',
    'uniclient_asr_empty_stream.json',
    'uniclient_asr_go_away.json',
    pytest.param('uniclient_asr_google_en_and_ru.json', marks=SKIP_GOOGLE_MARK),
    'uniclient_asr_options_mime.json',
    'uniclient_asr_grammar_onthefly.json',
    'uniclient_asr_manual_punctuation.json',
    'uniclient_asr_multi_utterance.json',
    'uniclient_asr_partial.json',
    'uniclient_asr_ru_desktopgeneral.json',
    'uniclient_asr_ru_dialogeneral.json',
    'uniclient_asr_ru_freeform.json',
    'uniclient_asr_ru_general.json',
    'uniclient_asr_ru_numbers_22.json',
    'uniclient_asr_ru_punctuation.json',
    'uniclient_asr_several.json',
    'uniclient_asr_slow.json',
    'uniclient_biometry_classify.json',
    'uniclient_biometry_score.json',
    pytest.param('uniclient_music2_asr.json', marks=pytest.mark.xfail),
    'uniclient_bad_streamcontrol.json',
    'uniclient_tts_bad_lang.json',
    'uniclient_tts_bad_voice.json',
    'uniclient_tts_chunking.json',
    'uniclient_tts_jane_en.json',
    'uniclient_tts_jane_uk.json',
    'uniclient_tts_krosh_ru.json',
    'uniclient_tts_list_voices.json',
    'uniclient_tts_omazh_ru_pcm8k.json',
    'uniclient_tts_omazh_ru_pcm_speed_0.3.json',
    'uniclient_tts_omazh_ru_pcm_speed_3.json',
    'uniclient_tts_omazh_tr.json',
    'uniclient_tts_realtime.json',
    'uniclient_tts_realtime_opus_stream.json',
    'uniclient_tts_sync_chunker.json',
    'uniclient_tts_shitovaus_ru.json',
    'uniclient_tts_fallback.json',
    'uniclient_tts_valtz_ru.json',
    'uniclient_vins_classification.json',
    pytest.param('uniclient_vins_music_input.json', marks=pytest.mark.xfail),
    'uniclient_spotter_check.json',
    'uniclient_spotter_check2.json',
    'uniclient_vins_no_prev_request_id.json',
    'uniclient_vins_text_input.json',
    'uniclient_vins_text_timings.json',
    'uniclient_vins_voice_input.json',
    'uniclient_vins_voice_timings.json',
    'uniclient_spotter_yandex.json',
    'uniclient_spotter_yandex2.json',
    'uniclient_spotter_pause_check.json',
    'uniclient_spotter_stop_check.json',
    'uniclient_spotter_stop_check2.json',
    'uniclient_spotter_fail.json',
    'uniclient_uaas_tests.json',
    'uniclient_invalidauth.json',
    # No tvm api in dev environment will make this test always fail
    pytest.param('uniclient_bad_messenger_auth.json', marks=pytest.mark.xfail),
    'uniclient_SoundRecorder.json',
    'uniclient_messenger_backend_versions.json',
    'uniclient_vins_single_utterance.json',
]

NO_HEADERS_SESSION_FILES = [
    'uniclient_no_uuid.json',
    'uniclient_bad_uuid.json',
    'uniclient_bad_auth_token.json',
]

NEXT_UNIPROXY_SESSION_FILES = [
    pytest.param('uniclient_asr_grammar_as_context.json', marks=pytest.mark.xfail),
    'uniclient_asr_multi_first.json',
    'uniclient_asr_multi_second.json',
    'uniclient_asr_multi_always_second.json',
    'uniclient_vins_merge_asr.json',
]

NEXT_TTS_SESSION_FILES = [
]

NEXT_YALDI_SESSION_FILES = [
]

NEXT_VINS_SESSION_FILES = [
]


class AsrTest:
    def __init__(self, lang, topic, filename, duration, norm_recog, recognize_payload):
        self.lang = lang
        self.topic = topic
        self.filename = filename
        self.duration = duration
        if isinstance(norm_recog, list):
            self.norm_recog = norm_recog + [(nr + ' ') for nr in norm_recog]
        else:
            self.norm_recog = [norm_recog, norm_recog + ' ']
        self.recognize_payload = recognize_payload

    def __str__(self):
        return '{}_{}_{}'.format(self.lang, self.topic, os.path.basename(self.filename))


def load_asr_tests():
    with open('asr_tests.json') as f:
        asr_tests = json.load(f)
    tests = []
    for lang, topic_dict in asr_tests.items():
        for topic, tests_list in topic_dict.items():
            for case in tests_list:
                if case[0].startswith('#'):
                    continue
                asr_test = AsrTest(lang, topic, case[0], case[2], case[1], case[3] if len(case) >= 4 else None)
                mark = []
                if config['asr'].get('backend', '') == 'yaldi':
                    if topic in ('buying',):
                        mark = pytest.mark.xfail
                tests.append(pytest.param(asr_test, id=str(asr_test), marks=mark))
    return tests


ASR_TESTS = load_asr_tests()


def impl_test_uniclient_asr(uniproxy_url, asr_test, api_key=None):
    filename = 'uniclient_asr_ru_general.json'
    with open(filename) as f:
        test_session = json.load(f)
    test_session[0]['timelimit'] = asr_test.duration * 1.5 + 3
    test_session[3]['message']['event']['payload']['lang'] = asr_test.lang
    test_session[3]['message']['event']['payload']['topic'] = asr_test.topic
    if asr_test.recognize_payload:
        test_session[3]['message']['event']['payload'].update(asr_test.recognize_payload)
    test_session[4]['filename'] = asr_test.filename
    test_session[5]['normalized_sample'] = [asr_test.norm_recog]
    del test_session[5]['recognition_sample']

    assert process_test_session(uniproxy_url, filename, api_key if api_key else config['key'], test_session) == []
