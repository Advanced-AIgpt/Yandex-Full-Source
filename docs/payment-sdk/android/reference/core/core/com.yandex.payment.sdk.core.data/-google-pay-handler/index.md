//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[GooglePayHandler](index.md)

# GooglePayHandler

[core]\
interface [GooglePayHandler](index.md)

Интерфейс обработчика GooglePay.

## Functions

| Name | Summary |
|---|---|
| [getRequestCode](get-request-code.md) | [core]<br>abstract fun [getRequestCode](get-request-code.md)(): Int<br>Желаемый request code для запуска Activity. |
| [googlePayActivityChallenge](google-pay-activity-challenge.md) | [core]<br>abstract fun [googlePayActivityChallenge](google-pay-activity-challenge.md)(resultStorage: [GooglePayActivityResultStorage](../-google-pay-activity-result-storage/index.md)): Activity<br>Необходимо вернуть текущую активити приложения, поверх неё будет запущена активити GooglePay. |
