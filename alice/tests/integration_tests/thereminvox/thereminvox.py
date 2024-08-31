import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class Intent:
    Disclaimer = 'theremin.station_disclaimer'
    HowManySounds = 'theremin.how_many_sounds'
    NotInstrument = 'theremin.not_instrument'
    Onboarding = 'theremin.onboarding_guide'
    OutOfRange = 'theremin.out_of_range'
    Play = 'theremin.play'
    Theremin = 'Theremin'
    WhatIsTheremin = 'theremin.what_is_theremin'


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
])
class TestThereminvox(object):

    owners = ('abc:yandexdialogs2',)

    def test_theremin_run(self, alice):
        response = alice('дай звук')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.Onboarding
        assert 'Включаю режим синтезатора.' in response.text

        response = alice('дай звук 1')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.Play
        assert 'Включаю' in response.text

        response = alice('дай звук гитары')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.Play
        assert 'Включаю' in response.text

        response = alice('хватит')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause

        response = alice('дай звук 59')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.OutOfRange
        assert response.text == 'Пока я знаю всего 58 звуков. Но это пока.' or \
            response.text == 'Моя библиотека пока не настолько большая. Но интересная! Я знаю 58 разных звуков.'

    def test_theremin_info(self, alice):
        response = alice('что такое режим синтезатора')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.WhatIsTheremin
        assert 'В Яндекс.Станции Мини вы можете играть мелодию движением руки.' in response.text

        response = alice('сколько звуков в режим синтезатора')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.HowManySounds
        assert 'В моей библиотеке 58 звуков.' in response.text or 'Я знаю 58 звуков.' in response.text

    def test_theremin_mielofon_off(self, alice):
        response = alice('дай звук тамагочи')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.NotInstrument
        assert 'Я такого не знаю.' in response.text or 'Увы, я не знаю такого звука.' in response.text


@pytest.mark.oauth(auth.Skills)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
])
class TestThereminvoxAuth(object):

    owners = ('abc:yandexdialogs2',)

    def test_theremin_mielofon_on(self, alice):
        response = alice('дай звук тамагочи')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.Play
        assert 'Включаю' in response.text


@pytest.mark.parametrize('surface', [surface.station, surface.smart_tv])
class TestThereminvoxStation(object):

    owners = ('abc:yandexdialogs2',)

    def test_theremin_run(self, alice):
        response = alice('дай звук')
        assert response.scenario == scenario.Theremin
        assert response.intent == Intent.Disclaimer
        assert 'Я умею включать разные звуки только в Яндекс.Станции Мини.' in response.text or \
            'Режим синтезатора работает только в Яндекс.Станции Мини.' in response.text
