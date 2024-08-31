import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


class TestPalmOnboarding(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-22
    https://testpalm.yandex-team.ru/testcase/alice-1122
    https://testpalm.yandex-team.ru/testcase/alice-1396
    https://testpalm.yandex-team.ru/testcase/alice-2862
    """

    owners = ('karina-usm',)

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_alice_22(self, alice):
        expected_phrases = [
            'Я могу подсказать дорогу, сыграть с тобой в игру или просто поболтать. Хочешь узнать ещё?',
            'Умею рассказывать о погоде, считать, а ещё включать сказки. Хочешь что-нибудь другое?',
            'Могу отвечать на разные вопросы. Например, сколько стоит велосипед. Хочешь узнать ещё?',
        ]

        response = alice('Что ты умеешь?')
        assert response.intent == intent.Onboarding
        first_reply = response.text
        assert first_reply in expected_phrases

        response = alice('Да')
        assert response.intent == intent.OnboardingNext
        second_reply = response.text
        assert second_reply in expected_phrases
        assert first_reply != second_reply

        response = alice('давай еще')
        assert response.intent == intent.OnboardingNext
        third_reply = response.text
        assert third_reply in expected_phrases
        assert first_reply != third_reply
        assert second_reply != third_reply

        response = alice('дальше')
        assert response.intent == intent.OnboardingNext
        fourth_reply = response.text
        assert fourth_reply in expected_phrases
        assert first_reply == fourth_reply

    @pytest.mark.parametrize('surface', [surface.navi])
    def test_alice_1122(self, alice):
        expected_phrases = [
            'Я могу найти нужный адрес и построить маршрут, поставить отметку на карте или показать пробки. Ещё могу '
            'рассказать анекдот, если вам станет скучно в дороге, а хотите — сыграем в игру или просто поболтаем. '
            'Спросите у меня что-нибудь.',

            'Я умею строить маршруты, могу посчитать, сколько осталось стоять в пробке, или найти нужный адрес. Ну а '
            'если вам станет скучно, со мной можно сыграть в игру или просто поболтать. Говорят, с чувством юмора у '
            'меня всё в порядке. Давайте проверим — спросите меня о чём-нибудь.',

            'Я строю маршруты, ищу адреса и парковки, ставлю отметки на карте. В общем, помогаю вам быстро добраться '
            'до места. Со мной и в дороге нескучно: могу рассказать анекдот, сыграть с вами в игру, да и '
            'просто поболтать. Давайте поговорим.',
        ]

        response = alice('Что ты умеешь?')
        assert response.intent == intent.WhatCanYouDo
        assert response.text in expected_phrases

        response = alice('Что еще ты умеешь?')
        assert response.intent == intent.WhatCanYouDo
        assert response.text in expected_phrases

    @pytest.mark.xfail(reason='ALICE-1239')
    @pytest.mark.parametrize('surface', [surface.automotive])
    def test_alice_1396(self, alice):
        response = alice('Что ты умеешь')
        assert response.intent == intent.WhatCanYouDo
        assert response.text == \
            'Яндекс.Авто это платформа встраиваемая в автомобили. '\
            'Теперь вы можете использовать Радио, Навигатор, '\
            'Погоду и Музыку — с единым интерфейсом и голосовым управлением.'

        response = alice('Расскажи про Яндекс.Авто')
        assert response.intent == intent.TellAboutYandexAuto
        assert response.text == 'Яндекс.Авто — это Навигатор, Музыка и другие сервисы Яндекса в автомобиле.'

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    def test_alice_2862(self, alice):
        response = alice('Что ты умеешь')
        assert response.intent == intent.Onboarding
        assert response.text.startswith('Я классная и могу помочь в поиске фильма или видеоролика.')

        response = alice('Дальше')
        assert response.intent == intent.OnboardingNext
        assert response.text.startswith('Могу включить конкретный телеканал - имейте в виду.')

        response = alice('Да')
        assert response.intent == intent.OnboardingNext
        assert response.text.startswith('Могу рассказать про погоду. Вдруг вам актуально.')

        response = alice('Еще')
        assert response.intent == intent.OnboardingNext
        assert response.text.startswith('Интересный факт: хотите узнать интересный факт - спросите меня.')

        response = alice('Продолжай')
        assert response.intent == intent.OnboardingNext
        assert response.text.startswith('Если вы вдруг заскучали, мы можем просто поболтать.')

        response = alice('Дальше')
        assert response.intent == intent.OnboardingNext
        assert response.text.startswith('Если что, могу подсказать имена актеров, мне не сложно.')

        response = alice('Да')
        assert response.intent == intent.OnboardingNext
        assert response.text.startswith('Я классная и могу помочь в поиске фильма или видеоролика.')


@pytest.mark.parametrize('surface', [surface.station])
class TestStationOnboarding(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1286
    """

    owners = ('glebovch',)

    @pytest.mark.parametrize('expected_directive', [
        pytest.param(directives.names.ShowTvGalleryDirective, id='native'),
        pytest.param(directives.names.MordoviaShowDirective, marks=pytest.mark.experiments('tv_channels_webview'), id='webview'),
    ])
    def test_alice_1286(self, alice, expected_directive):
        response = alice('Что по тв')
        assert response.directive.name == expected_directive

        response = alice('Помощь')
        assert response.text.startswith('Вы можете поставить таймер или будильник. Например, скажите: '
                                        '"Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".')

        response = alice('Включи номер 2')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.type == 'tv_stream'

        response = alice('Помощь')
        assert response.text.startswith('Вы можете поставить таймер или будильник. Например, скажите: '
                                        '"Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".')
