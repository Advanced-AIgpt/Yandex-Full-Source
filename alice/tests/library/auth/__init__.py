# По вопросам продления подписок следует заводить тикеты в очередь дежурных медиабиллинга - MBDUTY

# пользователь с аккаунтом в Яндексе
Yandex = 'robot-alice-integration-tests'

# пользователь с аккаунтом в Яндексе и подпиской Яндекс.Плюс
# с настроенным радио и историей прослушивания
YandexPlus = 'robot-alice-tests-plus'

# пользователь с аккаунтом в Яндексе и подпиской Яндекс.Плюс
# в другой стране (в Испании)
YandexPlusForeign = 'robot-alice-tests-plus-foreign'

# пользователь с аккаунтом в Яндексе и подпиской Яндекс.Плюс+Амедитека
Amediateka = 'robot-alice-tests-amediateka'

# пользователь с аккаунтом в Яндексе и являющийся сотрудником Яндекса
YandexStaff = 'yndx-alice-zero-testing'

# пользователи с аккаунтом в Яндексе и Умным домом
Smarthome = 'robot-alice-smarthome-test'
SmarthomeOther = 'robot-alice-smarthome-other'

# пользователь для сценария голового опроса состояний устройств Умного дома
# состояния его устройств не должны меняться, чтобы не сбивались ответы при опросе
SmartHomeVoiceQueries = 'robot-alice-smarthome-vq'

# пользователь с аккаунтом в Яндексе и настроенными навыками
Skills = 'robot-alice-skills'


# горячий пользователь в news.api (не использовать для кликов в новостные сюжеты, чтобы ничего не сбивать новостям)
NewsAPIHotUser = 'robot-news-api-ovechkin'


RobotMarketNoOrders = 'no-orders1'  # без истории заказов
RobotMarketNoBeruOrders = 'no-orders4'  # без истории заказов на Беру
RobotMarketNoPhoneAndOrders = 'no-phone-and-orders2'  # без истории заказов и телефона в паспорте
RobotMarketWithDelivery = 'with-orders'  # в истории заказов только товары с курьерской доставкой
RobotMarketWithDeliveryAndPickup = 'alice-pvz-kur2'  # в истории заказов товары с курьерской доставкой и самовывозом
RobotMarketWithPickup = 'alice-pvz3'  # в истории заказов товары с курьерской доставкой и самовывозом
RobotTaxi = 'alice.taxi.test.0'  # привязан московский номер, указаны точки Дом и Работа
RobotTestShoppingList = 'robot-test-shopping-list'


# пользователь еды с историей заказов
RobotEater = 'robot-alice-eater'


# пользователь с двумя девайсами и подпиской Яндекс.Плюс
# первый девайс (feedface-e8a2-4439-b2e7-000000000001.yandexstation_2) находится в комнате 'Спальня',
# второй (feedface-e8a2-4439-b2e7-000000000002.unknown_platform) на 'Кухне'
# оба девайся входят в группу 'Группа 1'
RobotMultiroom = 'robot-alice-hw-tests-plus'


# Пользователи с контактной книгой
RobotPhoneCaller = 'robot-alice-phone-caller'
RobotPhoneCallerFat4k = 'robot-alice-phone-caller-fat4k'
RobotPhoneCallerFat8k = 'robot-alice-phone-caller-fat8k'


# mock пользователи для CI тестов в библиотеки mark
FAKE_FOR_CI_TEST = 'FAKE_FOR_CI_TEST'
FAKE_FOR_CI_TEST_2 = 'FAKE_FOR_CI_TEST_2'
