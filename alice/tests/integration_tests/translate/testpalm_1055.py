import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.navi,
    surface.searchapp,
    surface.station,
    surface.watch,
])
class TestPalmTranslatePhrases(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1055
    https://testpalm.yandex-team.ru/testcase/alice-1504
    https://testpalm.yandex-team.ru/testcase/alice-1557
    https://testpalm.yandex-team.ru/testcase/alice-1873
    https://testpalm.yandex-team.ru/testcase/alice-2070
    """

    owners = ('sparkle',)

    @pytest.mark.parametrize('command, text_input, text_output, voice_output', [
        ('переведи слово {text}', 'стул', 'chair', 'chair'),
        ('как переводится {text}', 'вот из зе файл', 'что это за файл', 'что это за файл'),
        ('как по-грузински будет {text}', 'пока', 'ხოლო', 'холо'),
        ('как будет {text} по гречески', 'молоко', 'γάλα', 'гала'),
        ('переведи {text}', 'что ты сегодня делала', 'what did you do today', 'what did you do today'),
        ('переведи с русского на немецкий {text}', 'на улице никого нет', 'niemand ist auf der Straße.', 'niemand ist auf der straße'),
        ('как будет по китайски {text}', 'чай', '茶', 'ча'),
        ('переведи с английского {text}', 'экзампл', 'пример', 'пример'),
        ('перевод {text} на французский', 'компьютер', 'ordinateur', 'ordinateur'),
        ('перевод {text}', 'энимал', 'животное', 'животное'),
    ])
    def test(self, alice, command, text_input, text_output, voice_output):
        response = alice(command.format(text=text_input))
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate

        slots = response.slots
        assert slots['text'].string == text_input
        assert slots['result'].string == text_output
        for word in voice_output.split():
            assert word in slots['voice'].string
