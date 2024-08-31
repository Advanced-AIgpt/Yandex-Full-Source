//[datasource](../../../../index.md)/[com.yandex.payment.sdk.datasource.bind.interfaces](../../index.md)/[CardScreen](../index.md)/[State](index.md)

# State

[datasource]\
sealed class [State](index.md)

Тип состояния экрана.

## Types

| Name | Summary |
|---|---|
| [Error](-error/index.md) | [datasource]<br>class [Error](-error/index.md)(**error**: [PaymentKitError](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-kit-error/index.md)) : [CardScreen.State](index.md)<br>Произошла ошибка. |
| [Idle](-idle/index.md) | [datasource]<br>object [Idle](-idle/index.md) : [CardScreen.State](index.md)<br>Ожидание ввода. |
| [Loading](-loading/index.md) | [datasource]<br>object [Loading](-loading/index.md) : [CardScreen.State](index.md)<br>Загрузка. |
| [SuccessBind](-success-bind/index.md) | [datasource]<br>class [SuccessBind](-success-bind/index.md)(**card**: [BoundCard](../../../../../core/core/com.yandex.payment.sdk.core.data/-bound-card/index.md)) : [CardScreen.State](index.md)<br>Успешная привязка карты. |
| [SuccessPay](-success-pay/index.md) | [datasource]<br>class [SuccessPay](-success-pay/index.md)(**pollingResult**: [PaymentPollingResult](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-polling-result/index.md)) : [CardScreen.State](index.md)<br>Успешная оплата новой картой. |

## Inheritors

| Name |
|---|
| [CardScreen.State](-idle/index.md) |
| [CardScreen.State](-loading/index.md) |
| [CardScreen.State](-success-bind/index.md) |
| [CardScreen.State](-success-pay/index.md) |
| [CardScreen.State](-error/index.md) |
