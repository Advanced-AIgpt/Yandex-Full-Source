# Prebuilt UI компоненты

Содержит различные кастомизируемые Prebuilt UI компоненты.

## Подключение
Для подключения добавьте зависимость от сабспеки `UI`
```
pod 'YandexPaymentSDK/UIStatic', '<version>'
```

## Компоненты для работы с данными банковских карт
Это [CardInput](../../ui/CardInput.md) для работы со вводом всех данных карты и [CvnInput](../../ui/CvnInput.md) для ввода только CVV/CVC кода при оплате существующей картой. Создавать их можно с помощью фабрики [PrebuiltUiFactory](../../ui/PrebuiltUiFactory.md).

## Вспомогательные компоненты
Это кнопка оплаты [PayButton](../../ui/PayButton.md), вебвью для 3ds и Тинькофф [PaymentSDKWebViewController](../../ui/PaymentSDKWebViewController.md), таблица для списка методов [SelectMethodTableView](../../ui/SelectMethodTableView.md).

## Примеры использования
Можно посмотреть [тут](https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/ios/examples/UsingPrebuiltUI) работу с банковскими картами, а [тут](https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/ios/examples/UsingMediators/UsingMediators/PaymentMethodsListViewContoller.swift) пример работы со списком методов и его расширением кастомными способами оплаты.
