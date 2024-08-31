# Библиотека медиаторов

Библиотека со вспомогательными медиаторами для различных случаев работы с PaymentSDK. Можно использовать напрямую, расширять или просто смотреть на примере, как устроен флоу.

## Подключение
Для подключения добавьте зависимость от сабспеки `Mediators`
```
pod 'YandexPaymentSDK/MediatorsStatic', '<version>'
```

## Медиаторы
Абстрактный [CardInputMediator](../../mediators/CardInputMediator.md) предназначен для работы с компонентом ввода данных карт, два его потомка [BindCardMediator](../../mediators/BindCardMediator.md) и [NewCardPayMediator](../../mediators/NewCardPayMediator.md)  - для привязки карт и оплаты новой картой соответственно.
Для простого процесса оплаты сделан [SimplePaymentMediator](../../mediators/SimplePaymentMediator.md).
