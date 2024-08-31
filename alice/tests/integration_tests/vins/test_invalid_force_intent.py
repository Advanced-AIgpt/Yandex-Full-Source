import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.searchapp,
    surface.station,
])
@pytest.mark.experiments(
    f'mm_scenario={scenario.AliceVins}',
    force_intent='some_invalid_intent',
)
class TestVinsInvalidForceIntent(object):

    owners = ('dan-anastasev',)

    server_error_messages = [
        'Прошу прощения, что-то сломалось.',
        'Произошла какая-то ошибка.',
        'Извините, что-то пошло не так.',
        'Даже идеальные помощники иногда ломаются.',
        'Мне кажется, меня уронили.',
        'О, кажется, мы с вами нашли во мне ошибку. Простите.',
        'Мы меня сломали, но я обязательно починюсь.',
    ]

    def test_invalid_force_intent(self, alice):
        response = alice('привет')

        assert any(server_error_message in response.text for server_error_message in self.server_error_messages)
