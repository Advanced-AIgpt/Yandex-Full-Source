import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestChangeTrack(object):

    owners = ('amullanurov',)

    def test_change_audio(self, alice):
        alice('включи джентельмены')
        response = alice('включи английскую озвучку')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.ChangeAudioDirective

    def test_change_subtitles(self, alice):
        alice('включи джентельмены')
        response = alice('включи субтитры')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.ChangeSubtitlesDirective

    def test_change_audio_and_subtitles(self, alice):
        alice('включи джентельмены')
        response = alice('включи английскую озвучку и русские субтитры')
        assert response.scenario == scenario.VideoCommand
        directive_names = {d.name for d in response.directives}
        assert directive_names == {directives.names.ChangeAudioDirective, directives.names.ChangeSubtitlesDirective}

    def test_change_audio_and_subtitles_nonexistent(self, alice):
        alice('включи джентельмены')
        response = alice('включи английскую озвучку и французские субтитры')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.ShowVideoSettingsDirective

    def test_change_using_numbers(self, alice):
        alice('включи джентельмены')
        response = alice('включи 2 и 5')
        assert response.scenario == scenario.VideoCommand
        directive_names = {d.name for d in response.directives}
        assert directive_names == {directives.names.ChangeAudioDirective, directives.names.ChangeSubtitlesDirective}

    @pytest.mark.oauth(auth.Amediateka)
    def test_off_russian_subtitles(self, alice):
        alice('включи сериал игра престолов')
        response = alice('включи английскую озвучку')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.ChangeAudioDirective
        response = alice('выключи субтитры')
        assert response.scenario == scenario.VideoCommand
        assert response.text == 'Простите, выключить субтитры для этой озвучки не получается. Выберете другую озвучку?'
        assert not response.directive


@pytest.mark.version(videobass=85)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestChangeTrackLegatus(object):

    owners = ('amullanurov',)

    def test_change_track(self, alice):
        response = alice('включи английскую озвучку')
        assert response.scenario == scenario.VideoCommand
        assert response.text in [
            'Я еще не научилась этому. Давно собираюсь, но все времени нет.',
            'Я пока это не умею.',
            'Я еще не умею это.',
            'Я не могу пока, но скоро научусь.',
            'Меня пока не научили этому.',
            'Когда-нибудь я смогу это сделать, но не сейчас.',
            'Надеюсь, я скоро смогу это делать. Но пока нет.',
            'Я не знаю, как это сделать. Извините.',
            'Так делать я еще не умею.',
            'Программист Алексей обещал это вскоре запрограммировать. Но он мне много чего обещал.',
            'К сожалению, этого я пока не умею. Но я быстро учусь.',
        ]
