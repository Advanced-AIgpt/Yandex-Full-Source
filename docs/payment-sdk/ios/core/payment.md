# Платежи

## Старт платежа
Для старта платежа используйте метод [startPayment](../../core/PaymentApi.md#startpaymentpaymenttoken​order​iscredit​completion​) с нужным вам [PaymentToken](../../core/Payment_Token.md). Если вы внутренний сервис, имеющий service token и работающий с Trust, то используйте трастовый `purchase_token` для этого, если же нужно проводить платежи через Яндекс.Оплаты - то `payment_token` из оплат.

{% note warning %}

PaymentSDK работает с одним платежом за раз - предыдущий должен быть или завершен с каким-либо статусом, или отменен через вызов [cancel](../../core/PaymentInstance.md#cancel).

{% endnote %}

{% note info %}

Под капотом этот вызов идёт в бэкенд PaymentSDK, который в свою очередь ходит в `start_payment` Траста, а так же в антифрод и другие сервисы, при необходимости.

{% endnote %}

После успешного завершения вы получите в переданный коллбек объект [PaymentInstance](../../core/PaymentInstance.md), используя который можно дослать выбранный платежный метод и провести платеж.

## Методы оплаты
Доступные методы оплаты можно получить двумя способами:
1. [PaymentApi.paymentMethods](../../core/PaymentApi.md#paymentmethodscompletion​) вернет доступные методы для заданного пользователя на заданном сервисе.
1. [PaymentInstance.methods](../../core/PaymentInstance.md#methods) вернет методы доступные для оплаты конкретной корзины.

Эти списки могут отличаться, так как для конкретной корзины могут быть доступны, например, дополнительные методы (типа кредитов Тинькофф). Это зависит от того, как вы создаёте корзину на оплату. Но, в целом, как правило, эти списки совпадают.

## Выбор метода и оплата

### Оплата существующей картой картой
Передайте соответствующий объект [Payment.Method.card](../../core/Payment_Method.md#card) в [pay](../../core/PaymentInstance.md#paymethod​overrideuseremail​completion​). Нужен ли будет ввод CVC/CVV кода можно узнать заранее с помощью метода [shouldShowCvv](../../core/PaymentInstance.md#shouldshowcvvcardid​) и отобразить компонент для его ввода. Общую схему можно посмотреть [тут](../../scheme.md).

### Оплата новой картой
Вызовите [pay](../../core/PaymentInstance.md#paymethod​overrideuseremail​completion) с [Payment.Method.newCard](../../core/Payment_Method.md#newcard) для этого. Для ввода данных карты вам нужно будет отобразить у себя [CardInput](../../ui/CardInput.md), схема работы [тут](../../scheme.md).
Также вы можете использовать медиатор [NewCardPayMediator](../../mediators/NewCardPayMediator.md) для упрощения процесса.

## Оплата через Систему Быстрых Платежей
Сначала задайте обработчик в [setSbpHandler](../../core/PaymentInstance.md#setsbphandlerhandler​). В этот обработчик придёт сформированный `URL`, с которым можно вызвать `UIApplication.shared.open(url)` для вызова банковского приложения. После этого можете вызывать [pay](../../core/PaymentInstance.md#paymethod​overrideuseremail​completion​) с параметром [Payment.Method.sbp](../../core/Payment_Method.md#sbp).
Если вы хотите работать с вызовом банковских приложений напрямую, тогда сначала получите их список через [getInstalledBankApps](../../core/PaymentApi.md#getinstalledbankappscompletion​) и затем, когда к вам придёт `URL` для оплаты - подмените схему в нём на схему нужного банка, плюс для работы этой схемы надо добавить список топа банков в plist приложения [см тут](../../paymentsdk/sbp.md#обнаружение-банковских-приложений-на-телефоне).

{% note warning %}

PaymentSDK не имеет возможности отследить отмену из банковского приложения - нужно будет вручную вызвать [cancel](../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/cancel.md).

{% endnote %}

## Оплата через кредиты Тинькофф
Этот способ сильно отличается от остальных. Корзины для кредитных оплат создаются специальным образом. После получения объекта [PaymentInstance](../../core/PaymentInstance.md) вам нужно отобразить страничку с кредитом от Тинькофф банка, её адрес можно получить из поля [creditFormUrl](../../core/Payment_Settings.md) в [settings](../../core/PaymentInstance.md#settings). Для отображения нужно использовать [PaymentSDKWebViewController](../../ui/PaymentSDKWebViewController.md).

Как только вы получите статус [SUCCESS](../../ui/TinkoffFormState.md#success) или [APPOINTED](../../ui/TinkoffFormState.md#appointed) - вызывайте [pay](../../core/PaymentInstance.md#paymethod​overrideuseremail​completion​) с [Payment.Method.tinkoffCredit](../../core/Payment_Method.md#tinkoffcredit). В отличие от остальных методов оплаты для кредитов имеет значение что именно пришло в [PaymentPollingResult](../../core/PaymentPollingResult.md). [SUCCESS](../../core/PaymentPollingResult.md#success) означает что кредит одобрен и оплата прошла, [WAIT_FOR_PROCESSING](../../core/PaymentPollingResult.md#waitforprocessing) означает, что назначена встреча с представителем банка.
