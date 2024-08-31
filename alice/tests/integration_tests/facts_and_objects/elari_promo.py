import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmFactsElariWatch(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-28
    '''

    owners = ('bstrukov', 'svetlana-yu')

    def testpalm_28(self, alice):
        response = alice('Сколько будет тридцать умножить на два')
        assert response.intent == intent.Calculator
        assert response.text == '60'

        response = alice('Что такое смешарики')
        assert response.intent == intent.ObjectAnswer
        assert ('мультипликационный сериал' in response.text.casefold() or
                'смешарики' in response.text.casefold() or
                'истории о дружбе и приключениях обаятельных круглых героев.' in response.text.casefold())

        response = alice('Найди карданный вал на жигули')
        assert response.intent == intent.Search or response.product_scenario == intent.ProtocolSearch
        assert response.text in [
            'Извините, у меня нет хорошего ответа.',
            'У меня нет ответа на такой запрос.',
            'Я пока не умею отвечать на такие запросы.',
            'Простите, я не знаю что ответить.',
            'Я не могу на это ответить.',
            'По вашему запросу я ничего не нашла.',
            'По вашему запросу ничего не нашлось.',
            'По вашему запросу не получилось ничего найти.',
            'По вашему запросу ничего найти не получилось.',
            'К сожалению, я ничего не нашла.',
            'К сожалению, ничего не нашлось.',
            'К сожалению, не получилось ничего найти.',
            'К сожалению, ничего найти не получилось.',
            'Ничего не нашлось.',
            'Я ничего не нашла.',
            'Я ничего не смогла найти.',
        ]

        response = alice('Сколько будет 17 плюс 7')
        assert response.intent == intent.Calculator
        assert response.text == '24'

        response = alice('Подскажи квадратный корень из восьми')
        assert response.intent == intent.Calculator
        assert response.text == 'примерно 2.8284'

        response = alice('Кто такие Фиксики')
        assert response.intent == intent.ObjectAnswer
        assert ('мультипликационный сериал' in response.text or
                'Фиксики- это маленькие человечки' in response.text or
                'мультфильм' in response.text)

        response = alice('Сколько сантиметров в метре')
        assert response.intent == intent.Factoid
        assert response.text == '1 метр - это 100 сантиметров'

        response = alice('Какие цвета в радуге?')
        assert response.intent == intent.Factoid
        assert 'красный' in response.text
        assert 'оранжевый' in response.text
        assert 'желтый' in response.text or 'жёлтый' in response.text
        assert 'зеленый' in response.text or 'зелёный' in response.text
        assert 'голубой' in response.text or 'индиго' in response.text
        assert 'синий' in response.text
        assert 'фиолетовый' in response.text

        response = alice('Сколько будет 12 умножить на 11')
        assert response.intent == intent.Calculator
        assert response.text == '132'

        response = alice('Кто такой Гоголь')
        assert response.intent in [intent.Factoid, intent.ObjectAnswer]
        assert 'литератур' in response.text

        response = alice('Сколько весит слон')
        assert response.intent == intent.Factoid
        assert response.text == 'от 2 до 7 тонн'

        response = alice('Кто такой йети')
        assert response.intent in [intent.Factoid, intent.ObjectAnswer]
        assert ('Снежный человек' in response.text or
                'Сне́жный челове́к' in response.text or
                'Мифическое человекообразное существо' in response.text or
                'Йети' in response.text)

        response = alice('Сколько весит кот')
        assert response.intent == intent.Factoid
        assert ('кг' in response.text or 'килограмм' in response.text)

        response = alice('Сколько весит кошка')
        assert response.intent == intent.Factoid
        assert ('кг' in response.text or 'килограмм' in response.text)

        response = alice('Сколько весит кит')
        assert response.intent in [intent.Factoid, intent.ObjectAnswer]
        assert ('тонн' in response.text or
                'кг' in response.text or
                'от 100 до 120 т' in response.text)

        response = alice('Откуда берутся дети')
        assert response.intent == intent.WhereDoChildrenComeFrom
