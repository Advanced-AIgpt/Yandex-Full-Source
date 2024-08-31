# Инициализация

## PaymentInitFactory
Сначала вам нужно создать экземпляр [PaymentInitFactory](../../core/core/com.yandex.payment.sdk.core/-payment-init-factory/index.md). При создании опционально можно указать тип используемого окружения [PaymentSdkEnvironment](../../core/core/com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md) и режим логирования [ConsoleLoggingMode](../../core/core/com.yandex.payment.sdk.core.data/-console-logging-mode/index.md), например:
```kotlin
PaymentInitFactory(
        applicationContext,
        PaymentSdkEnvironment.TESTING,
        ConsoleLoggingMode.ENABLED,
)
```
После этого можно создать билдер для основного класса [PaymentApi](../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md) вызвав метод [createBuilder](../../core/core/com.yandex.payment.sdk.core/-payment-init-factory/create-builder.md).

## PaymentApi.Builder
Билдер позволяет задать множество различных опций, обязательными из них являются:
* [Payer](../../core/core/com.yandex.payment.sdk.core.data/-payer/index.md) - объект с данными о пользователе
* [Merchant](../../core/core/com.yandex.payment.sdk.core.data/-merchant/index.md) - объект с данными о сервисе
* [PaymentCallbacks](../../core/core/com.yandex.payment.sdk.core.data/-payment-callbacks/index.md) - базовые коллбеки для работы с [PaymentApi](../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md)
Пример:
```kotlin
val payer = Payer("OAuth token", "example@yandex.ru", 999999, null, null, null)
val merchant = Merchant("trust_service_token_string")
val paymentApi = factory
    .createBuilder()
        .payer(payer)
        .merchant(merchant)
        .paymentCallbacks(callbacksHolder)
        .build()
```
После вызова build() вы получите основной объект [PaymentApi](../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md)