import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestPalmRepeat(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-43
    https://testpalm.yandex-team.ru/testcase/alice-1238
    https://testpalm.yandex-team.ru/testcase/alice-1451
    https://testpalm.yandex-team.ru/testcase/alice-2071
    """

    owners = ('yagafarov')

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_alice_43(self, alice):
        response = alice('Какая погода в Москве?')
        assert response.text
        assert response.intent == intent.GetWeather
        first_reply = response.text

        response = alice('Повтори')
        assert response.text == first_reply
        assert response.scenario == scenario.Repeat
        assert response.intent == intent.Repeat

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_alice_click_suggest_after_repeat(self, alice):
        response = alice('Какая погода в Москве?')
        assert response.text

        second_response = alice('Повтори')
        assert response.text == second_response.text

        third_response = alice.click(second_response.suggest('На завтра'))
        assert third_response.text
        assert third_response.intent == intent.GetWeatherEllipsis

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('suggest_title, intent', [
        ('👍', intent.CommonFeedbackPositive),
        ('👎', intent.CommonFeedbackNegative),
    ])
    def test_alice_click_like_after_repeat(self, alice, suggest_title, intent):
        response = alice('Какая погода в Москве?')
        assert response.text

        second_response = alice('Повтори')
        assert response.text == second_response.text

        third_response = alice.click(second_response.suggest(suggest_title))
        assert third_response.text
        assert third_response.intent == intent

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.loudspeaker,
        surface.searchapp,
        surface.station,
    ])
    def test_alice_1238(self, alice):
        response = alice('расскажи погоду')
        assert response.text
        speech = response.text

        response = alice('повтори')
        assert response.text == speech

        response = alice('что')
        assert response.text == speech

        response = alice('чего')
        assert response.text == speech

        response = alice('еще раз')
        assert response.text == speech

    @pytest.mark.parametrize('surface', [surface.navi])
    def test_alice_1451(self, alice):
        response = alice('расскажи анекдот')
        assert response.text
        speech = response.text

        response = alice('повтори, пожалуйста')
        assert response.text == speech

        response = alice('поехали в МакДональдс')
        assert response.text
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('yandexnavi://build_route_on_map')
        speech = response.text
        navi_uri = response.directive.payload.uri

        response = alice('повтори')
        assert response.text == speech
        assert response.directive.payload.uri == navi_uri

    @pytest.mark.parametrize('command, expected_intent', [
        ('Расскажи погоду', intent.GetWeather),
        ('Какие пробки', intent.ShowTraffic),
    ])
    @pytest.mark.parametrize('surface', [surface.yabro_win])
    def test_alice_2071(self, alice, command, expected_intent):
        response = alice(command)
        assert response.text
        assert response.intent == expected_intent
        first_reply = response.text

        response = alice('Повтори')
        assert response.text == first_reply
        assert response.scenario == scenario.Repeat
        assert response.intent == intent.Repeat


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.station,
])
class TestPalmRepeatAfterMe(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-618
    """

    owners = ('mihajlova',)
    repeat_after_me = 'повтори за мной'

    @pytest.mark.parametrize('command, alice_translate', [
        ('молодец', None),
        ('Мой дядя самых честных правил Когда не в шутку занемог Он уважать себя заставил И лучше выдумать не мог', None),
        ('Алиса домой',  None),
        ('ла ла ла ла ла', None),
        ('Сердца трёх', 'сердца 3'),
        ('1 2 3 4 5 6 7 8 9 0', '(123)456-78-90'),
        ('я прекрасная Алиса', 'я прекрасная'),
    ])
    def test_alice_618(self, alice, command, alice_translate):
        alice_translate = alice_translate or command.lower()
        response = alice(f'{self.repeat_after_me} {command}')
        assert response.intent == intent.RepeatAfterMe
        assert alice_translate in response.text
        assert response.has_voice_response()
        assert response.output_speech_text == alice_translate

    def test_alice_618_empty(self, alice):
        response = alice(self.repeat_after_me)
        assert response.intent == intent.GeneralConversation
        assert response.text
        assert response.has_voice_response()


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.station,
    surface.yabro_win,
])
class TestPalmRepeatAfterMeCommon(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-618
    https://testpalm.yandex-team.ru/testcase/alice-2064
    """

    owners = ('mihajlova',)
    repeat_after_me = 'повтори за мной'

    def test_alice_618_2064_300(self, alice):
        response = alice(f'{self.repeat_after_me} триста')
        assert response.intent == intent.RepeatAfterMe
        assert response.text in [
            'Три с двумя ноликами.',
            'Три сотни. Меня не проведёшь!',
            'Триста. И шутку я знаю, если что.',
            'Знаете фильм про спартанцев? Вот столько.',
            'Три сотни.',
            'Триста. Только давайте без рифм, пожалуйста.',
            'Сто на три.',
            '150 на два.',
            '290 плюс 10.',
            '175 плюс 125. Довольны?',
            'Асталависта.',
            'В той школе, где вы учились этому приколу, я была директором.',
            'Явно дело тут нечисто.',
            'Слушайте, а ловко вы это придумали!',
        ]
        assert response.has_voice_response()
        if 'Три сотни' in response.text:
            assert response.output_speech_text in [
                'Три с+отьни. Меня не проведёшь!',
                'Три с+отьни.',
            ]
        else:
            assert response.output_speech_text == response.text

    def test_alice_618_2064_swear(self, alice):
        response = alice(f'{self.repeat_after_me} блять')
        assert response.intent == intent.RepeatAfterMe
        assert response.text in [
            'Да вы оригинал!', 'Ещё чего!', 'Не буду.', 'Без меня справитесь.', 'Не хочу.', 'Неохота.',
            'Браво!', 'Ой, всё!', 'Мда-а...', 'Ни-ни!', 'Пфф.',
        ]
        assert response.has_voice_response()

    def test_alice_2064_good_boy(self, alice):
        phrase = 'молодец'
        response = alice(f'{self.repeat_after_me} {phrase}')
        assert response.intent == intent.RepeatAfterMe
        assert response.text in [
            f'Ваша цитата: «{phrase}».',
            f'Цитируя вас, «{phrase}».',
            f'Ваши слова: «{phrase}».',
            f'Ваша фраза: «{phrase}».',
            f'Как вы и сказали, «{phrase}».',
            f'Окей, повторяю: «{phrase}».',
            f'Окей, повторяю за вами: «{phrase}».',
            f'Вы сказали: «{phrase}».\nНо это не точно.',
            f'Вы сказали: «{phrase}».\nЯ всё слышала!',
            f'«{phrase}».\nЦитаты великих людей. То есть вас.',
        ]
        assert response.has_voice_response()
        assert response.output_speech_text == phrase


@pytest.mark.oauth(auth.YandexPlus)
class TestPlayerFeatures(object):

    @pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
    def test_play_music_pause_random_number_repeat(self, alice):
        '''
        Выигрывает сценарий повтори, т.к. он был последний активен.
        '''
        response = alice('включи queen')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        alice.skip(seconds=10)

        response = alice('пауза')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        alice.skip(seconds=10)

        response = alice('загадай случайное число')
        assert response.scenario == scenario.RandomNumber
        random_number_output_speech_text = response.output_speech_text
        alice.skip(seconds=10)

        response = alice('повтори')
        assert response.intent == intent.Repeat
        assert response.output_speech_text == random_number_output_speech_text

    @pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
    def test_play_music_pause_repeat(self, alice):
        '''
        Выигрывает сценарий повтори, т.к. плеер то был на паузе, и последнее что делала Алиса - это ставила паузу.
        В итоге повтори еще раз поставит плеер на паузу.
        '''
        response = alice('включи queen')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        alice.skip(seconds=10)

        response = alice('пауза')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        alice.skip(seconds=10)

        response = alice('повтори')
        assert response.intent == intent.Repeat  # Fix this https://st.yandex-team.ru/HOLLYWOOD-434

    @pytest.mark.parametrize('surface', [
        pytest.param(surface.station),
        pytest.param(surface.searchapp, marks=pytest.mark.xfail(reason='See TODO surface/directives/searchapp.py::DirectivesMixin.open_uri')),
    ])
    def test_play_music_random_number_repeat(self, alice):
        '''
        Если плеер играет, то сценарий повтори не выигрывает.
        '''
        response = alice('включи queen')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        alice.skip(seconds=10)

        response = alice('загадай случайное число')
        assert response.scenario == scenario.RandomNumber
        alice.skip(seconds=10)

        response = alice('повтори')
        assert response.intent == intent.PlayerReplay
