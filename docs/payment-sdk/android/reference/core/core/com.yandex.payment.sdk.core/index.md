//[core](../../index.md)/[com.yandex.payment.sdk.core](index.md)

# Package com.yandex.payment.sdk.core

## Types

| Name | Summary |
|---|---|
| [ForPaymentSdkMigration](-for-payment-sdk-migration/index.md) | [core]<br>annotation class [ForPaymentSdkMigration](-for-payment-sdk-migration/index.md)<br>Используется для временных интерфейсов на время миграции PaymentSDK на использование [PaymentApi](-payment-api/index.md). |
| [MetricaSwitch](-metrica-switch/index.md) | [core]<br>enum [MetricaSwitch](-metrica-switch/index.md) : Enum<[MetricaSwitch](-metrica-switch/index.md)> |
| [PaymentApi](-payment-api/index.md) | [core]<br>interface [PaymentApi](-payment-api/index.md)<br>Главный интерфейс для работы с API - позволяет работать с платежами, привязками банковских карт и GooglePay. |
| [PaymentCompletion](index.md#152061939%2FClasslikes%2F-2113150450) | [core]<br>typealias [PaymentCompletion](index.md#152061939%2FClasslikes%2F-2113150450)<[T](index.md#152061939%2FClasslikes%2F-2113150450)> = Result<[T](index.md#152061939%2FClasslikes%2F-2113150450), [PaymentKitError](../com.yandex.payment.sdk.core.data/-payment-kit-error/index.md)><br>Алиас для основных коллбеков API |
| [PaymentInitFactory](-payment-init-factory/index.md) | [core]<br>class [PaymentInitFactory](-payment-init-factory/index.md)(**context**: Context, **environment**: [PaymentSdkEnvironment](../com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md), **consoleLoggingMode**: [ConsoleLoggingMode](../com.yandex.payment.sdk.core.data/-console-logging-mode/index.md), **metricaInitMode**: [MetricaInitMode](../com.yandex.payment.sdk.core.data/-metrica-init-mode/index.md))<br>Базовая фабрика для инициализации, используется для задания окружения [PaymentSdkEnvironment](../com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md) и режима логирования [ConsoleLoggingMode](../com.yandex.payment.sdk.core.data/-console-logging-mode/index.md). |
| [PciDssWarning](-pci-dss-warning/index.md) | [core]<br>annotation class [PciDssWarning](-pci-dss-warning/index.md)<br>Вы не должны использовать ничего из того, что помеченно этой аннотацией напрямую, иначе будет очень много вопросов при аудите PCI DSS. |

## Properties

| Name | Summary |
|---|---|
| [REGION_ID_RUSSIA](-r-e-g-i-o-n_-i-d_-r-u-s-s-i-a.md) | [core]<br>const val [REGION_ID_RUSSIA](-r-e-g-i-o-n_-i-d_-r-u-s-s-i-a.md): Int = 225<br>Константа, соответствует Российской Федерации в геобазе, является дефолтным значением. |
