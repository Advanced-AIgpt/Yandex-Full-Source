//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[Builder](index.md)

# Builder

[core]\
class [Builder](index.md)

Билдер для создания объекта [PaymentApi](../index.md).

## Functions

| Name | Summary |
|---|---|
| [appInfo](app-info.md) | [core]<br>fun [appInfo](app-info.md)(appInfo: [AppInfo](../../../com.yandex.payment.sdk.core.data/-app-info/index.md)): [PaymentApi.Builder](index.md)<br>Информация о турбоаппе ПП/Браузера. |
| [browserCards](browser-cards.md) | [core]<br>fun [browserCards](browser-cards.md)(browserCards: List<[BrowserCard](../../../com.yandex.payment.sdk.core.data/-browser-card/index.md)>): [PaymentApi.Builder](index.md)<br>Передать карты, хранимые в браузере. |
| [build](build.md) | [core]<br>fun [build](build.md)(): [PaymentApi](../index.md)<br>Собрать [PaymentApi](../index.md). |
| [enableCashPayments](enable-cash-payments.md) | [core]<br>fun [enableCashPayments](enable-cash-payments.md)(enableCashPayments: Boolean): [PaymentApi.Builder](index.md)<br>Разрешить наличные как метод оплаты. |
| [exchangeOauthToken](exchange-oauth-token.md) | [core]<br>fun [exchangeOauthToken](exchange-oauth-token.md)(exchangeOauthToken: Boolean): [PaymentApi.Builder](index.md)<br>Нужно ли обменивать в паспорте переданный токен на токен со скоупом платежей. |
| [forceCVV](force-c-v-v.md) | [core]<br>fun [forceCVV](force-c-v-v.md)(forceCVV: Boolean): [PaymentApi.Builder](index.md)<br>Всегда запрашивать CVV/CVC код при оплате картой. |
| [googlePayData](google-pay-data.md) | [core]<br>fun [googlePayData](google-pay-data.md)(data: [GooglePayData](../../../com.yandex.payment.sdk.core.data/-google-pay-data/index.md)?): [PaymentApi.Builder](index.md)<br>Данные для GooglePay в режиме Payment Gateway. |
| [googlePayHandler](google-pay-handler.md) | [core]<br>fun [googlePayHandler](google-pay-handler.md)(googlePayHandler: [GooglePayHandler](../../../com.yandex.payment.sdk.core.data/-google-pay-handler/index.md)?): [PaymentApi.Builder](index.md)<br>Обработчик для GooglePay. |
| [gpayAllowedCardNetworks](gpay-allowed-card-networks.md) | [core]<br>fun [gpayAllowedCardNetworks](gpay-allowed-card-networks.md)(allowedCardNetworks: [GooglePayAllowedCardNetworks](../../../com.yandex.payment.sdk.core.data/-google-pay-allowed-card-networks/index.md)): [PaymentApi.Builder](index.md)<br>Настройка доступных платежный систем Google Pay. |
| [merchant](merchant.md) | [core]<br>fun [merchant](merchant.md)(merchant: [Merchant](../../../com.yandex.payment.sdk.core.data/-merchant/index.md)): [PaymentApi.Builder](index.md)<br>Данные о сервисе. |
| [passportToken](passport-token.md) | [core]<br>fun [passportToken](passport-token.md)(passportToken: String?): [PaymentApi.Builder](index.md)<br>Паспортный токен для вебавторизации. |
| [payer](payer.md) | [core]<br>fun [payer](payer.md)(payer: [Payer](../../../com.yandex.payment.sdk.core.data/-payer/index.md)): [PaymentApi.Builder](index.md)<br>Данные о пользователе. |
| [paymentCallbacks](payment-callbacks.md) | [core]<br>fun [paymentCallbacks](payment-callbacks.md)(callbacks: [PaymentCallbacks](../../../com.yandex.payment.sdk.core.data/-payment-callbacks/index.md)): [PaymentApi.Builder](index.md)<br>Основные коллбеки [PaymentApi](../index.md). |
| [paymentMethodsFilter](payment-methods-filter.md) | [core]<br>fun [paymentMethodsFilter](payment-methods-filter.md)(filter: [PaymentMethodsFilter](../../../com.yandex.payment.sdk.core.data/-payment-methods-filter/index.md)): [PaymentApi.Builder](index.md)<br>Фильтр доступных методов оплаты. |
| [regionId](region-id.md) | [core]<br>fun [regionId](region-id.md)(regionId: Int): [PaymentApi.Builder](index.md)<br>Регион пользователя. |
