import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.searchapp, surface.yabro_win])
class TestTranslationErrors(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-1459
    '''

    owners = ('sparkle',)

    @pytest.mark.parametrize('command', [
        'переведи с русского на немецкий',
        'как будет по немецки',
    ])
    def test_translation_ok(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'translate.yandex.ru' in response.directive.payload.uri
        assert response.suggest('Открой Переводчик')

    def test_translation_same_language(self, alice):
        response = alice('Переведи стол на русский')
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        assert response.text in [
            'Это и есть нужный вам язык, русский.',
            'Сдаётся мне, это и есть русский.',
            'Погодите. Эта фраза уже на русском.',
            'Если я не ошибаюсь, это уже русский.',
        ]

    def test_translation_transliteration(self, alice):
        response = alice('Как будет table по-английски')
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        assert response.text in [
            'Это и есть нужный вам язык, английский.',
            'Сдаётся мне, это и есть английский.',
            'Погодите. Эта фраза уже на английском.',
            'Если я не ошибаюсь, это уже английский.',
        ]

    def test_translation_unsupported_language(self, alice):
        response = alice('Переведи ковер на древнегреческий')
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        assert response.text in [
            'Я не знаю древнегреческого. Когда-нибудь изучу.',
            'Мне не знаком древнегреческий. Такое бывает.',
            'У меня тут 95 языков, но древнегреческого среди них нет. Увы.',
        ]
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.suggest('Открой Переводчик')
