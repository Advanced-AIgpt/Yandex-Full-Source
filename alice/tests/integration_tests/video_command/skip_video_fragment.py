import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestSkipVideoFragment(object):

    owners = ('amullanurov',)

    def test_skip_intro_fragment(self, alice):
        response = alice('включи рик и морти 3 сезон 1 серия')
        assert response.scenario == scenario.Video

        alice.skip(seconds=95)
        response = alice('пропусти заставку')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.PlayerRewindDirective

    def test_skip_unskippable_fragment(self, alice):
        response = alice('включи рик и морти 3 сезон 1 серия')
        assert response.scenario == scenario.Video

        alice.skip(seconds=200)
        response = alice('перемотай заставку')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text in [
            'Простите, не могу перемотать этот фрагмент. Может быть, он важный?',
            'Простите, не могу это перемотать. Хочу посмотреть.',
            'Простите, не перематывается. Буду это чинить.',
        ]

    def test_skip_credits(self, alice):
        response = alice('включи рик и морти 3 сезон 1 серия')
        assert response.scenario == scenario.Video

        alice.skip(minutes=22, seconds=18)
        response = alice('пропусти титры')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.VideoPlayDirective


@pytest.mark.version(videobass=85)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestSkipVideoFragmentLegatus(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command', [
        pytest.param('пропусти заставку', id='skip_intro'),
        pytest.param('пропусти титры', id='skip_credits')
    ])
    def test_skip_credits(self, alice, command):
        response = alice(command)
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
