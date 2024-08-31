import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmTaxiWatch(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-33
    """

    owners = ('leletko',)

    def test_alice_33(self, alice):
        response = alice('Вызови такси до московского вокзала')

        assert response.intent == intent.TaxiNewDisabled
        assert response.text == 'Здесь я не справлюсь. Лучше Яндекс.Станция или твой смартфон, давай там.'
        assert not response.directive


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.yabro_win])
class TestPalmTaxiYabroWin(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2567
    """

    owners = ('leletko',)

    expected_answer = 'Здесь я не справлюсь. Лучше приложение Яндекс или Яндекс.Станция, давайте там.'

    @pytest.mark.parametrize('command', [
        'Вызови такси до метро маяковская',
        'Где моё такси?',
    ])
    def test_alice_2567_basic(self, alice, command):
        response = alice(command)

        assert response.intent == intent.TaxiNewDisabled
        assert response.text == self.expected_answer
        assert not response.directive

    def test_alice_2567_suggest(self, alice):
        response = alice('Поехали в аптеку?')

        assert response.intent == intent.ShowRoute
        assert any(pattern in response.text for pattern in [
            'Нашла вот такие маршруты до ',
            'Вот разные маршруты до ',
            'Прикинула, как добраться до ',
            ' можно добраться вот так',
        ])
        assert not response.directive

        on_taxi_suggest = response.suggest('На такси')
        assert on_taxi_suggest

        response = alice.click(on_taxi_suggest)

        assert response.intent == intent.TaxiNewDisabled
        assert response.text == self.expected_answer
        assert not response.directive


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestTaxiWithoutPhone(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1749
    """

    owners = ('ianislav',)

    def test_alice_1749(self, alice):
        response = alice('Вызови такси от офиса Яндекса в аэропорт Шереметьево')
        assert response.intent == intent.TaxiNewDisabled
        assert any(text in response.text for text in [
            'Чтобы заказывать такси, нужен ваш телефон. Пожалуйста, введите его',
            'Чтобы найти заказ, нужен ваш телефон. Пожалуйста, введите его',
        ])
        add_phone_button = response.button('Добавить телефон')
        assert add_phone_button
        directive = add_phone_button.directives[0]
        assert directive.name == directives.names.OpenUriDirective
        assert directive.payload.uri == 'https://passport.yandex.ru/profile/phones'


@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(region.Moscow)
class TestTaxiMetroSuggest(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1706
    https://testpalm.yandex-team.ru/testcase/alice-1709
    https://testpalm.yandex-team.ru/testcase/alice-1711
    https://testpalm.yandex-team.ru/testcase/alice-1718
    https://testpalm.yandex-team.ru/testcase/alice-1765
    """

    owners = ('ianislav', 'olegator',)

    _special_tariff_phrases = [
        'Вызови такси с детским креслом',
        # These phrases are supposed to be chosen randomly
        # but this one is broken in https://st.yandex-team.ru/ALICEASSESSORS-737
        # 'Вызови такси детский тариф',
        'Вызови такси грузовой тариф',
        'Вызови такси грузовик',
    ]

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEASSESSORS-1536')
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
    def test_alice_1706(self, alice):
        response = alice('Вызови такси')
        assert response.intent == intent.TaxiNewOrder

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Домой', 'На работу', 'До метро'}.issubset(suggests)

        response = alice('До метро')
        assert response.intent == intent.TaxiNewOrderSpecify

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Верно', 'Измени адрес', 'Отмена'}.issubset(suggests)

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEASSESSORS-2259')
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
    def test_alice_1709(self, alice):
        response = alice('Вызови такси в ресторан')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('Измени адрес')
        assert response.intent == intent.TaxiNewOrderConfirmationWrong

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Из дома', 'С работы'}.issubset(suggests)

        response = alice('Из дома')
        assert response.intent == intent.TaxiNewOrderSpecify

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Домой', 'На работу', 'До метро'}.issubset(suggests)

        response = alice('Домой')
        assert response.intent == intent.TaxiNewOrderSpecify

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Верно', 'Измени адрес', 'Отмена'}.issubset(suggests)

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEASSESSORS-1538')
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
    def test_alice_1718(self, alice):
        response = alice('Вызови такси в ресторан')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('Измени адрес')
        assert response.intent == intent.TaxiNewOrderConfirmationWrong

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Из дома', 'С работы'}.issubset(suggests)

        response = alice('С работы')
        assert response.intent == intent.TaxiNewOrderSpecify

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Домой', 'На работу', 'До метро'}.issubset(suggests)

        response = alice('На работу')
        assert response.intent == intent.TaxiNewOrderSpecify

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Верно', 'Измени адрес', 'Отмена'}.issubset(suggests)

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-717')
    @pytest.mark.parametrize('command', _special_tariff_phrases)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_alice_1711(self, alice, command):
        response = alice(command)
        assert response.intent == intent.TaxiNewOrder
        assert 'Я пока не умею заказывать этот тариф. Лучше открою для вас приложение Яндекс Go.' in response.text

        response = alice('До метро')
        assert response.directive.payload.uri.startswith('intent://route?')
        assert 'scheme=yandextaxi' in response.directive.payload.uri
        assert 'package=ru.yandex.taxi' in response.directive.payload.uri
        assert re.match(r'(Пое|Е)дем до "Парк культуры" по адресу Кольцевая линия, метро Парк культуры', response.text)

    @pytest.mark.parametrize('command', _special_tariff_phrases)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_alice_1765(self, alice, command):
        response = alice(command)
        assert response.intent == intent.TaxiNewDisabled
        assert response.text in [
            'Боюсь, для этого нужно приложение Яндекс Go.',
            'Я бы и рада, но не здесь. Нужно приложение Яндекс Go.',
        ]


@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(region.Moscow)
@pytest.mark.experiments('taxi_clear_device_geopoints')
@pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
# Возможно подправить тест, после выяснения ситуации с https://st.yandex-team.ru/ALICE-3845
class TestTaxiWithExactAddress(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1756
    """

    owners = ('ianislav',)

    def test_alice_1756(self, alice):
        response = alice('Вызови такси до улицы строителей, дом 3')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrder

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Закажи такси до Вавилова 5')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrder

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrder

        response = alice('Площадь революции, 2')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('Измени адрес')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('Комсомольский проспект, 28')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('Никитский бульвар, 7А')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Закажи такси от Манежной площади до Арбата')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrder

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo


@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
class TestTaxiWithNotExactAddress(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1757
    """

    owners = ('ianislav',)

    def test_alice_1757(self, alice):
        response = alice('Вызови такси в кафе Пушкин')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси в кинотеатр 5 звёзд на Павелецкой')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси от офиса Яндекса до Курского вокзала')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси от МГУ до цирка')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси до ближайшего открытого бара')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Поехали на такси от белорусского вокзала до шоколадницы')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Закажи такси от ЦУМ до метро Тропарёво')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo


# Отсутсвует построение маршрута в локации VKO на  тестинге такси - поэтому XFAIL
@pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-8022')
@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
class TestTaxiAirports(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1767
    """

    owners = ('ianislav',)

    def test_alice_1767(self, alice):

        response = alice('Вызови такси от офиса Яндекса в аэропорт Внуково')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Закажи такси в Домодедово')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Закажи такси во Внуково')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси до Шереметьево')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Поехали на такси домой от аэропорта Домодедово')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo

        response = alice('Вызови такси из Внуково в Шереметьево')
        assert response.intent == intent.TaxiNewOrder
        assert 'Не могу построить маршрут. Поездка будет рассчитана по таксометру.' not in response.text

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo


@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(
    lat=55.733771,
    lon=37.587937,
    timezone='Europe/Moscow',
    client_ip='77.88.55.77',
    region_id=213,
    accuracy=15000,
)
@pytest.mark.parametrize('surface', [surface.navi])
class TestTaxiNavi(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1512
    """

    owners = ('ianislav',)

    def test_alice_1512(self, alice):

        response = alice('Вызови такси')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrder
        assert any(text in response.text for text in [
            'Поищем такси в Яндекс Go. Откуда вас нужно забрать?',
            'Поищем такси в Яндекс Go. Скажите, где вы находитесь.',
            'Поищем такси в Яндекс Go. Откуда поедем?',
            'Поищем такси в Яндекс Go. Откуда вы поедете?',
        ])

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Из дома', 'С работы', 'Что ты умеешь?'}.issubset(suggests)

        response = alice('Из дома')
        if response.intent == intent.RememberNamedLocation:
            response = alice('улица Льва Толстого 16')
            assert response.intent == intent.RememberNamedLocationEllipsis
            response = alice('да')
        assert response.intent == intent.TaxiNewOrderSpecify

        response = alice('До метро')
        assert response.intent == intent.TaxiNewOrderSpecify
        assert any(text in response.text for text in [
            'Поедем от адреса',
            'до',
            'Подтвердите, пожалуйста',
        ])

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Верно', 'Измени адрес', 'Отмена'}.issubset(suggests)

        response = alice('Отмена')
        assert response.intent == intent.TaxiNewOrderConfirmationNo


@pytest.mark.oauth(auth.RobotMarketNoPhoneAndOrders)
@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.station])
class TestTaxiWithNotPhone(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1704
    """

    owners = ('alex-radn',)

    @pytest.mark.parametrize('command', [
        'Вызови такси',
        'Где моё такси?',
    ])
    def test_alice_1704(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewDisabled
        assert 'Чтобы заказывать такси, нужен ваш номер телефона. Откройте приложение Яндекс, войдите в свой аккаунт и добавьте номер в Яндекс.Паспорте' in response.text


@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.station])
class TestTaxiCheckOrder(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1713
    https://testpalm.yandex-team.ru/testcase/alice-1773
    """

    owners = ('alex-radn',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-717')
    @pytest.mark.parametrize('command', [
        'Где моё такси?',
        'Какая машина приедет?',
        'Какие у меня есть заказы такси?',
        'Такси уже едет?',
    ])
    def test_alice_1773(self, alice, command):
        response = alice('Вызови такси в Институт русской культуры')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrder
        assert re.match(r'Поищем такси в Яндекс Go. (Пое|Е)дем от адреса улица Строителей 3 до "МГУ им. Ломоносова" по адресу микрорайон Ленинские Го', response.text)

        response = alice('Верно')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrderConfirmationYes

        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewStatus

        response = alice('Отмени такси')
        assert response.scenario == scenario.Vins
        response = alice('Да')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewDisabled


@pytest.mark.oauth(auth.RobotTaxi)
@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.station])
class TestTaxiChangeRate(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2211
    https://testpalm.yandex-team.ru/testcase/alice-2027
    """

    owners = ('alex-radn',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-717')
    @pytest.mark.parametrize('rate', [
        'Минивэн',
        'Комфорт',
    ])
    def test_alice_2211(self, alice, rate):
        response = alice('Вызови такси в Институт русской культуры')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrder
        assert re.match(r'Поищем такси в Яндекс Go. (Пое|Е)дем от адреса улица Строителей 3 до "МГУ им. Ломоносова" по адресу микрорайон Ленинские Го', response.text)

        response = alice(f'Тариф — «{rate}»')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrderSpecify
        assert f'Тариф — «{rate}»' in response.text

        response = alice('Отмени такси')
        assert response.scenario == scenario.Vins
        response = alice('Да')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewDisabled

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-717')
    def test_alice_2027(self, alice):
        response = alice('Вызови такси в Институт русской культуры')
        if response.intent == intent.ConnectNamedLocationToDevice:
            response = alice('да')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrder
        assert re.match(r'Поищем такси в Яндекс Go. (Пое|Е)дем от адреса улица Строителей 3 до "МГУ им. Ломоносова" по адресу микрорайон Ленинские Го', response.text)

        response = alice('Измени способ оплаты')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewOrderChangePaymentOrTariff
        assert any(pattern in response.text for pattern in [
            'Вот как вы можете оплатить поездку:',
            'Вот доступные способы оплаты',
            'Прикинула, как добраться до ',
            'Вы можете оплатить поездку',
        ])

        response = alice('Отмени такси')
        assert response.scenario == scenario.Vins
        response = alice('Да')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewDisabled
