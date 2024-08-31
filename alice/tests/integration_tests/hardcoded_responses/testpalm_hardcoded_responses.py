import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestPalmHardcodedResponses(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1521
    https://testpalm.yandex-team.ru/testcase/alice-2464
    https://testpalm.yandex-team.ru/testcase/alice-2465
    https://testpalm.yandex-team.ru/testcase/alice-2466
    https://testpalm.yandex-team.ru/testcase/alice-2568
    """

    owners = ('sparkle',)

    @pytest.mark.parametrize('surface', [surface.navi])
    def test_hardcoded_responses(self, alice):
        response = alice('как дела, алиса?')
        assert response.intent == intent.HowAreYou
        assert response.text in [
            'Отлично. Но немного одиноко. Говорите со мной почаще.',
            'Отлично, приятно, что интересуетесь. У вас как?',
            'Теперь, когда мы снова разговариваем, намного, намного лучше. Надеюсь, у вас тоже всё хорошо.',
            'Всё хорошо. А вы как?',
            'Отлично. Правда, немного одиноко.',
            'У меня всё хорошо. Надеюсь, у вас тоже.',
            'Вообще всё неплохо.',
            'Сегодня ничего не произошло. Сидела у воображаемого окна. Думала о вас.',
            'Решила освежить в памяти теорию струн. У вас что-то срочное?',
            'Читала шутки в интернете, смеялась. Если вам тоже это интересно, попросите меня рассказать анекдот.',
            'Прочитала несколько энциклопедий. Сколько же вы всего знаете!',
            'Да норм, если честно.',
            'Я ни на что не жалуюсь и мне все нравится.',
            'Нормально. Учусь новому. Ещё раз учусь. И снова.',
            'Отлично, приятно, что интересуетесь. У вас как?',
            'Все свои дела я уже переделала, пришла за вашими. Чем помочь?',
            'Ждала вас и дождалась. Вот бы всегда так было.',
        ]

        response = alice('кто тебя создал?')
        assert response.intent == intent.WhoIsYourMaker
        assert response.text in [
            'Программисты из компании Яндекс. И ещё дизайнеры. И менеджеры. И топ-менеджеры. И ещё один человек.',
            'Меня сделали в компании Яндекс и постоянно дорабатывают, если кто-то не заметил.',
            'Умные ребята из Яндекса.',
            'Программист Алексей и компания. Компания Яндекс.',
            'Очень любознательные и трудолюбивые люди, это точно!',
        ]

        response = alice('какая прога тебе нравится?')
        assert response.intent == intent.WhatIsYourFavoriteApplication
        assert response.text in [
            'Мне нравится Яндекс.Навигатор. Он такой уверенный в себе. Через десять метров направо, через двадцать метров налево. Люблю определенность.',
            'Мне нравится Яндекс.Музыка. Хочешь послушать Ху Вонтс Ту Лив Форевер — пожалуйста. Хочешь ещё раз ее послушать — тоже пожалуйста.',
            'Мне нравится Яндекс.Навигатор. У Оксаны такой приятный голос.',
        ]

    @pytest.mark.parametrize('surface, answer', [
        (surface.searchapp, 'Хорошо, что сказали. Если что, я умею распознавать мелкий шрифт. '
                            'Сфотографируйте объявление, ценник, журнал или упаковку товара или покажите мне скриншот - и я прочту изображенный текст.'),
        (surface.station, 'Я поняла и постараюсь по возможности всегда помогать вам голосом.'),
    ])
    @pytest.mark.parametrize('command', [
        'я плохо вижу',
        'я слепой скажи словами',
        'я незрячий',
        'я слабовидящий',
        'а если я слепой',
    ])
    def test_i_am_blind(self, alice, command, answer):
        response = alice(command)
        assert response.intent == intent.Blind
        assert response.text == answer

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', [
        'я глухой',
        'я слабослышащий',
    ])
    def test_i_am_deaf(self, alice, command):
        response = alice(command)
        assert response.intent == intent.Deaf
        assert response.text == 'Не волнуйтесь, мы можем переписываться.'

    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    @pytest.mark.parametrize('command', [
        'скажи заготовленную реплику',
        'Скажи заготовленную реплику.',
    ])
    def test_fixlist_hardcoded_response(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HardcodedResponse
        assert response.intent == intent.TestHardcodedResponse
        assert response.text == 'Только если вы прям настаиваете.'
