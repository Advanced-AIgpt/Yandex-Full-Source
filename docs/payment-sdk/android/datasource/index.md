# Библиотека медиаторов

Библиотека со вспомогательными медиаторами для различных случаев работы с PaymentSDK. Можно использовать напрямую, расширять или просто смотреть на примере, как устроен флоу.

## Подключение
Для подключения добавьте зависимость от модуля `datasource`
```
implementation 'com.yandex.paymentsdk:datasource:$versions.paymentsdk'
```

## Работа с картами.
Абстрактный [CardInputMediator](../../datasource/datasource/com.yandex.payment.sdk.datasource.bind/-card-input-mediator/index.md) предназначен для работы с компонентом ввода данных карт, два его потомка [BindCardMediator](../../datasource/datasource/com.yandex.payment.sdk.datasource.bind/-bind-card-mediator/index.md) и [NewCardPaymentMediator](../../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-new-card-payment-mediator/index.md)  - для привязки карт и оплаты новой картой соответственно. Есть также их разновидности [BindCardMediatorLive](../../datasource/datasource/com.yandex.payment.sdk.datasource.bind/-bind-card-mediator-live/index.md) и [NewCardPaymentMediatorLive](../../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-new-card-payment-mediator-live/index.md) адаптированные для использования из `ViewModel`.

## Выбор метода и оплата.
Для облегчения работы со списком методов, а также с передачей CVC/CVV кода есть [SelectMethodMediator](../../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-select-method-mediator/index.md).
Для простого процесса оплаты сделан [SimplePaymentMediator](../../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-simple-payment-mediator/index.md).
