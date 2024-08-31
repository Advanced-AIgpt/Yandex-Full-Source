//[datasource](../../../../index.md)/[com.yandex.payment.sdk.datasource.payment.interfaces](../../index.md)/[PaymentScreen](../index.md)/[State](index.md)

# State

[datasource]\
sealed class [State](index.md)

Состояние экрана с оплатой.

## Types

| Name | Summary |
|---|---|
| [Error](-error/index.md) | [datasource]<br>class [Error](-error/index.md)(**error**: [PaymentKitError](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-kit-error/index.md)) : [PaymentScreen.State](index.md)<br>Произошла ошибка. |
| [Idle](-idle/index.md) | [datasource]<br>object [Idle](-idle/index.md) : [PaymentScreen.State](index.md)<br>Ожидание ввода. |
| [Loading](-loading/index.md) | [datasource]<br>object [Loading](-loading/index.md) : [PaymentScreen.State](index.md)<br>Загрузка. |
| [Success](-success/index.md) | [datasource]<br>class [Success](-success/index.md)(**pollingResult**: [PaymentPollingResult](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-polling-result/index.md)) : [PaymentScreen.State](index.md)<br>Успешная оплата. |

## Inheritors

| Name |
|---|
| [PaymentScreen.State](-idle/index.md) |
| [PaymentScreen.State](-loading/index.md) |
| [PaymentScreen.State](-success/index.md) |
| [PaymentScreen.State](-error/index.md) |
