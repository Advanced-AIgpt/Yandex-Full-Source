import re


# Text answer is common for all responses: only titles, no intro and conclusion.
# API must have from 5 (search_app) to 7 (speakers) stories.
RE_SMI_TEXT_REPLY = re.compile('(.+\n\n){4,6}.+')
# Wizard is disabled in news
WIZARD_TEXT_REPLY = 'К сожалению, я не смогла найти новостей по данному запросу.'
STUB_TEXT_REPLY = (
    'Чтобы слушать новости, попросите меня рассказать новости из конкретного источника '
    'или выберите его в настройках. '
    'Для этого скажите мне в приложении Яндекс: Алиса, настрой новости.'
)

RE_WIZARD_TEXT_REPLY = re.compile(WIZARD_TEXT_REPLY)
RE_STUB_TEXT_REPLY = re.compile(STUB_TEXT_REPLY)
RE_NONEWS_TEXT_REPLY = re.compile(f'({WIZARD_TEXT_REPLY}|{STUB_TEXT_REPLY})')


def _assert_text_and_voice_match_pattern(response, text_regex, voice_regex=None):
    # Search app's text is '...' when return divcards, easiest way to handle it.
    assert response.text == '...' or text_regex.fullmatch(response.text) is not None
    if voice_regex:
        assert response.has_voice_response()
        assert voice_regex.fullmatch(response.output_speech_text) is not None


def check_smi_text(response):
    _assert_text_and_voice_match_pattern(response, RE_SMI_TEXT_REPLY)


def check_wizard_text(response):
    _assert_text_and_voice_match_pattern(response, RE_WIZARD_TEXT_REPLY)


def check_stub_text(response):
    _assert_text_and_voice_match_pattern(response, RE_STUB_TEXT_REPLY)


def check_nonews_text(response):
    _assert_text_and_voice_match_pattern(response, RE_NONEWS_TEXT_REPLY)
