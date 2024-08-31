import re

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.navi,
    surface.station,
    surface.watch,
])
class TestPalmTranslateErrors(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1057
    https://testpalm.yandex-team.ru/testcase/alice-1503
    https://testpalm.yandex-team.ru/testcase/alice-1558
    https://testpalm.yandex-team.ru/testcase/alice-1874
    """

    owners = ('sparkle',)

    @pytest.mark.parametrize('command', [
        'переведи с русского на немецкий',
        'как будет по немецки',
    ])
    def test_no_text(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        assert response.text in [
            'Не поняла. Давайте снова. Пример: «переведи слово "кошка" на испанский».',
            'Не поняла. Повторите, пожалуйста. Пример: «переведи слово "собака" на немецкий».',
            'А что именно перевести? Попробуйте спросить по-другому. Например: «как будет "стол" по-английски?»',
        ]

    @pytest.mark.parametrize('unsupported_lang', [
        'древнегреческий',
        'арамейский',
        'абхазский',
        'табасаранский',
        'древнерусский',
        'эрзянский',
    ])
    def test_unsupported_lang(self, alice, unsupported_lang):
        response = alice(f'переведи на {unsupported_lang} ковер')
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate

        assert response.slots['lang_dst'].string == f'unsupported {unsupported_lang}'
        assert (re.match(r'Я не знаю .*\. Когда-нибудь изучу\.', response.text) or
                re.match(r'У меня тут 95 языков, но .* среди них нет\. Увы\.', response.text) or
                re.match(r'Мне не знаком .*\. Такое бывает.', response.text))

    @pytest.mark.parametrize('command, lang', [
        ('переведи стол на {lang}', 'русский'),
        ('переведи table на {lang}', 'английский'),
        ('переведи du hast на {lang}', 'немецкий'),
    ])
    def test_already_translated(self, alice, command, lang):
        response = alice(command.format(lang=lang))
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate

        slots = response.slots
        assert slots['lang_src'].string == lang
        assert slots['lang_src'].string == slots['lang_dst'].string
        assert ('result' not in slots or slots['result'].string == 'null')
