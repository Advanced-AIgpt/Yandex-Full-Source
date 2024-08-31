# Работа с GooglePay

Помимо обычной оплаты есть ещё несколько вспомогательных методов для GooglePay, можно посмотреть в интерфейсе [GooglePayApi](../../core/core/com.yandex.payment.sdk.core/-payment-api/-google-pay-api/index.md).

## Подготовка

### GooglePayHandler
Если вы хотите использовать у себя GooglePay, то при построении [PaymentApi](../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md) обязательно задайте реализацию [GooglePayHandler](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-handler/index.md).

В методе [googlePayActivityChallenge](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-handler/google-pay-activity-challenge.md) нужно будет вернуть вашу активити, поверх которой будет запущена активити GooglePay. Также в этот коллбек придёт объект [GooglePayActivityResultStorage](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-activity-result-storage/index.md), в который нужно будет передать полученный результат из сработавшего по завершении `onActivityResult`.

В методе [getRequestCode](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-handler/get-request-code.md) нужно будет вернуть код, с которым будет запущен UI GooglePay, по этому же коду нужно будет в `onActivityResult` выделить результат работы GooglePay для передачи в объект [GooglePayActivityResultStorage](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-activity-result-storage/index.md).

### Параметры GooglePay
Также обязательно нужно задать либо [GooglePayData.Gateway](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-data/-gateway/index.md), либо [GooglePayData.Direct](../../core/core/com.yandex.payment.sdk.core.data/-google-pay-data/-direct/index.md) в зависимости от вашей схемы работы с GooglePay.

{% note warning %}

Доступность метода определяется также конфигом бэкенда `PaymentSDK`. Обратитесь в саппорт для включения.

{% endnote %}

## Получить токен GooglePay
С помощью метода [makeGooglePayToken](../../core/core/com.yandex.payment.sdk.core/-payment-api/-google-pay-api/make-google-pay-token.md) в GooglePay будет запрошен токен для заказа. Может быть полезно для привязки платежей и досписаний.

## Привязать токен GooglePay в Траст
Для этого вызовите метод [bindGooglePayToken](../../core/core/com.yandex.payment.sdk.core/-payment-api/-google-pay-api/bind-google-pay-token.md) с полученным ранее токеном GooglePay.

## Проверка доступности GooglePay на устройстве
Используйте метод [isGooglePayAvailable](../../core/core/com.yandex.payment.sdk.core/-payment-api/-google-pay-api/is-google-pay-available.md). Проверяет только доступность оплаты на самом устройстве, что необходимо, но не достаточно для работы с GooglePay, см. выше.