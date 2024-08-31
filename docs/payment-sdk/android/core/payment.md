# Платежи

## Старт платежа
Для старта платежа используйте метод [startPayment](../../core/core/com.yandex.payment.sdk.core/-payment-api/start-payment.md) с нужным вам [PaymentToken](../../core/core/com.yandex.payment.sdk.core.data/-payment-token/index.md). Если вы внутренний сервис, имеющий service token и работающий с Trust, то используйте трастовый `purchase_token` для этого, если же нужно проводить платежи через Яндекс.Оплаты - то `payment_token` из оплат.

{% note warning %}

PaymentSDK работает с одним платежом за раз - предыдущий должен быть или завершен с каким-либо статусом, или отменен через вызов [cancel](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/cancel.md).

{% endnote %}

{% note info %}

Под капотом этот вызов идёт в бэкенд PaymentSDK, который в свою очередь ходит в `start_payment` Траста, а так же в антифрод и другие сервисы, при необходимости.

{% endnote %}

После успешного завершения вы получите в переданный коллбек объект [Payment](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/index.md), используя который можно дослать выбранный платежный метод и провести платеж.

## Методы оплаты
Доступные методы оплаты можно получить двумя способами:
1. [PaymentApi#paymentMethods](../../core/core/com.yandex.payment.sdk.core/-payment-api/payment-methods.md) вернет доступные методы для заданного пользователя на заданном сервисе.
1. [Payment#methods](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/methods.md) вернет методы доступные для оплаты конкретной корзины.

Эти списки могут отличаться, так как для конкретной корзины могут быть доступны, например, дополнительные методы (типа кредитов Тинькофф). Это зависит от того, как вы создаёте корзину на оплату. Но, в целом, как правило, эти списки совпадают.

## Выбор метода и оплата

### Оплата существующей картой картой
Передайте соответствующий объект [PaymentMethod.Card](../../core/core/com.yandex.payment.sdk.core.data/-payment-method/-card/index.md) в [pay](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/pay.md). Нужен ли будет ввод CVC/CVV кода можно узнать заранее с помощью метода [shouldShowCvv](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/should-show-cvv.md) и отобразить компонент для его ввода. Общую схему можно посмотреть [тут](../../scheme.md).

### Оплата новой картой
Вызовите [pay](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/pay.md) с [PaymentMethod.NewCard](../../core/core/com.yandex.payment.sdk.core.data/-payment-method/-new-card/index.md) для этого. Для ввода данных карты вам нужно будет отобразить у себя [CardInputView](../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md), схема работы [тут](../../scheme.md).
Также вы можете использовать медиатор [NewCardPaymentMediator](../../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-new-card-payment-mediator/index.md) для упрощения процесса.

## Оплата с GooglePay
Передайте [PaymentMethod.GooglePay](../../core/core/com.yandex.payment.sdk.core.data/-payment-method/-google-pay/index.md) в [pay](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/pay.md). Вы также должны заранее при сборке [PaymentApi](../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md) передать всё необходимое для работы GooglePay, смотрите раздел Подготовка [тут](google.md).

## Оплата через Систему Быстрых Платежей
Сначала задайте обработчик в [setSbpHandler](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/set-sbp-handler.md). В этот обработчик придёт сформированный `Intent`, с которым можно вызвать `startActivity` для вызова банковского приложения. После этого можете вызывать [pay](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/pay.md) с параметром [PaymentMethod.Sbp](../../core/core/com.yandex.payment.sdk.core.data/-payment-method/-sbp/index.md).
{% note warning %}

PaymentSDK не имеет возможности отследить отмену из банковского приложения - нужно будет вручную вызвать [cancel](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/cancel.md).

{% endnote %}

## Оплата через кредиты Тинькофф
Этот способ сильно отличается от остальных. Корзины для кредитных оплат создаются специальным образом. После получения объекта [Payment](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/index.md) вам нужно отобразить страничку с кредитом от Тинькофф банка, её адрес можно получить из поля [creditFormUrl](../../core/core/com.yandex.payment.sdk.core.data/-payment-settings/credit-form-url.md) в [settings](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/settings.md). Для отображения нужно использовать [PaymentSdkTinkoffWebView](../../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-tinkoff-web-view/index.md).

Как только вы получите статус [SUCCESS](../../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-tinkoff-web-view/-tinkoff-state/-s-u-c-c-e-s-s/index.md) или [APPOINTED](../../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-tinkoff-web-view/-tinkoff-state/-a-p-p-o-i-n-t-e-d/index.md) - вызывайте [pay](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/pay.md) с [PaymentMethod.TinkoffCredit](../../core/core/com.yandex.payment.sdk.core.data/-payment-method/-tinkoff-credit/index.md). В отличие от остальных методов оплаты для кредитов имеет значение что именно пришло в [PaymentPollingResult](../../core/core/com.yandex.payment.sdk.core.data/-payment-polling-result/index.md). [SUCCESS](../../core/core/com.yandex.payment.sdk.core.data/-payment-polling-result/-s-u-c-c-e-s-s/index.md) означает что кредит одобрен и оплата прошла, [WAIT_FOR_PROCESSING](../../core/core/com.yandex.payment.sdk.core.data/-payment-polling-result/-w-a-i-t_-f-o-r_-p-r-o-c-e-s-s-i-n-g/index.md) означает, что назначена встреча с представителем банка.
