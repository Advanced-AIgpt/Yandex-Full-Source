import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


REPORT = 'Маруся лучше Алисы'


@pytest.mark.voice
class TestBugreport(object):

    owners = ('olegator', 'ardulat')

    commands = [
        'отправь багрепорт',
        'свяжись с поддержкой',
        'долорес режим анализа',
    ]

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', commands)
    def test_searchapp(self, alice, command):
        response = alice(command)
        assert response.intent == intent.Bugreport
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'Открываю страницу обратной связи' in response.text

        response = alice(REPORT)
        assert response.intent == intent.GeneralConversation

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command', commands)
    def test_unsupported(self, alice, command):
        response = alice(command)
        assert response.intent == intent.Bugreport
        assert not response.directive
        assert 'Простите, я не могу помочь вам с этим' in response.text

        response = alice(REPORT)
        assert response.intent == intent.GeneralConversation

    @pytest.mark.experiments('debug_mode')
    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command', commands)
    def test_supported_flag(self, alice, command):
        response = alice(command)
        assert response.intent == intent.Bugreport
        assert response.directive.name == directives.names.SendBugReportDirective
        assert 'Пожалуйста, расскажите об ошибке подробнее' in response.text

        response = alice(REPORT)
        assert response.intent == intent.BugreportContinue
        assert 'Добавьте еще что' in response.text

        response = alice(f'{command} {REPORT}')
        assert response.intent == intent.BugreportContinue
        assert 'Добавьте еще что' in response.text

        response = alice('стоп')
        assert response.intent == intent.BugreportDeactivate
        assert 'Багрепорт отправлен' in response.text
