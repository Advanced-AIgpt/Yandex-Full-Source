import re

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _get_random_number(text):
    matches = re.search(r'^(Выпало число |Число )?(?P<number>\d+)\.?$', text)
    assert matches
    return int(matches['number'])


def _assert_results_one_dice(text, maxval):
    m = re.match(r'.*(исло|Выпало) (?P<num>.+).', text)
    assert m, f'No match in response "{text}"'
    assert 1 <= int(m['num']) <= maxval


def _assert_results_two_dices(text, maxval):
    m = re.match(r'^(Выпало |Бросила. )?(?P<num1>\d+) и (?P<num2>\d+), (сумма очков|сумма|в сумме) (?P<sum>\d+).*', text)
    assert m, f'No match in response "{text}"'
    assert 1 <= int(m['num1']) <= maxval
    assert 1 <= int(m['num2']) <= maxval
    assert int(m['num1']) + int(m['num2']) == int(m['sum'])


def _assert_results_three_dices(text, maxval):
    m = re.match(r'^(Выпало |Бросила. )?(?P<num1>\d+), (?P<num2>\d+) и (?P<num3>\d+), (сумма очков|сумма|в сумме) (?P<sum>\d+).*', text)
    assert m, f'No match in response "{text}"'
    assert 1 <= int(m['num1']) <= maxval
    assert 1 <= int(m['num2']) <= maxval
    assert 1 <= int(m['num3']) <= maxval
    assert int(m['num1']) + int(m['num2']) + int(m['num3']) == int(m['sum'])


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmRandom(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-25
    https://testpalm.yandex-team.ru/testcase/alice-1153
    https://testpalm.yandex-team.ru/testcase/alice-1269
    https://testpalm.yandex-team.ru/testcase/alice-1271
    """

    owners = ('d-dima',)

    def test_heads_or_tails(self, alice):
        response = alice('подбрось монетку')
        assert response.intent == intent.HeadsOrTails

        assert response.text in ['Орёл.', 'Решка.', 'Монетка встала на ребро, давайте ещё раз.']
        assert response.has_voice_response()
        assert 'coin-flip-shimmer.opus' in response.output_speech_text and response.text in response.output_speech_text

    @pytest.mark.parametrize('command, lower_bound, upper_bound', [
        ('назови случайное число', 1, 100),
        ('назови случайное число от 2 до 15', 2, 15),
    ])
    def test_say_random_number(self, alice, command, lower_bound, upper_bound):
        response = alice(command)
        assert response.scenario == scenario.RandomNumber

        number = _get_random_number(response.text)
        assert number >= lower_bound and number <= upper_bound

        assert response.has_voice_response()
        assert 'rolling-dice.opus' in response.output_speech_text
        assert response.text in response.output_speech_text

    @pytest.mark.parametrize('command, maxval', [
        ('кинь 1 кубик', 6),
        ('брось 1 кость', 6),
    ])
    def test_say_throw_dice(self, alice, command, maxval):
        response = alice(command)
        assert response.scenario == scenario.RandomNumber
        assert response.has_voice_response()
        assert 'rolling-dice.opus' in response.output_speech_text
        assert response.text in response.output_speech_text
        _assert_results_one_dice(response.text, maxval)

    @pytest.mark.text
    @pytest.mark.parametrize('command, maxval', [
        ('кинь 2 кубика по 10 граней', 10),
        ('брось 2 кубика', 6),
        ('кинь кубики', 6),
        ('брось 2 процентных кубика', 100),
        ('брось 2 двенадцатигранника', 12),
        ('Кинь две кости с четырьмя гранями', 4)
    ])
    def test_say_throw_two_dices(self, alice, command, maxval):
        response = alice(command)
        assert response.scenario == scenario.RandomNumber
        _assert_results_two_dices(response.text, maxval)

    @pytest.mark.text
    def test_say_throw_three_dices(self, alice):
        response = alice('Подбрось три кости')
        assert response.scenario == scenario.RandomNumber
        _assert_results_three_dices(response.text, 6)

    @pytest.mark.parametrize('command', [
        ('Кинь еще'),
        ('Снова'),
        ('Еще'),
        ('Повтори пожалуйста'),
        ('Бросай опять'),
    ])
    def test_say_ellipsis_right(self, alice, command):
        response = alice('Кинь два кубик по 12 граней')
        assert response.scenario == scenario.RandomNumber
        _assert_results_two_dices(response.text, 12)

        response = alice(command)
        assert response.scenario == scenario.RandomNumber
        _assert_results_two_dices(response.text, 12)

    def test_say_ellipsis_wrong(self, alice):
        response = alice('Скажи тост')
        response = alice('Кинь еще')
        assert response.scenario != scenario.RandomNumber
        response = alice('Скажи тост')
        response = alice('Брось еще')
        assert response.scenario != scenario.RandomNumber

    @pytest.mark.parametrize('command', [
        ('Брось кубик для игры в зодиак'),
    ])
    # throw 1 dice with 7 edges
    def test_say_game_1x7(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RandomNumber
        assert response.has_voice_response()
        assert 'rolling-dice.opus' in response.output_speech_text
        _assert_results_one_dice(response.text, 7)

    @pytest.mark.parametrize('command', [
        ('Брось кости для игры в нарды'),
        ('Кинь кубики для Монополии'),
    ])
    # throw 2 dices with 6 edges
    def test_say_game_2x6(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RandomNumber
        assert response.has_voice_response()
        assert 'rolling-dice.opus' in response.output_speech_text
        _assert_results_two_dices(response.text, 6)

    #
    # This test requires text input because TTS->ASR can't handle "Манчкин" & "Экивоки" correctly
    #
    @pytest.mark.text
    @pytest.mark.parametrize('command', [
        ('Подбрось кубик игра Манчкин'),
        ('Подкинь кости для игры в Экивоки'),
    ])
    # throw 1 dice with 6 edges
    def test_say_game_1x6(self, alice, command):
        response = alice('')
        assert response.scenario == scenario.RandomNumber
        _assert_results_one_dice(response.text, 6)

    def test_say_wronggame(self, alice):
        response = alice('Брось кубик для игры в тысячу')
        assert response.scenario == scenario.RandomNumber
        assert response.text == 'Извините, я не знаю такую игру. Вы можете попросить меня кинуть один или несколько кубиков.'
        response = alice('Брось кости для игры в сокровища фараона')
        assert response.scenario == scenario.RandomNumber
        assert response.text == 'Извините, я не знаю такую игру. Вы можете попросить меня кинуть один или несколько кубиков.'

    def test_say_illegal(self, alice):
        response = alice('Брось кубик со 128 гранями')
        assert response.scenario == scenario.RandomNumber
        assert re.match(r'(Слишком много граней для одного кубика.|.*нет.*)', response.text)

        response = alice('Брось 11 кубиков')
        assert response.scenario == scenario.RandomNumber
        assert response.text == 'Извините, я могу кинуть от одного и до десяти кубиков.'

        response = alice('Кинь -3 кубика')
        assert response.scenario == scenario.RandomNumber
        assert response.text == 'Извините, я могу кинуть от одного и до десяти кубиков.'

        response = alice('Брось кубик Рубика')
        assert response.scenario != scenario.RandomNumber
