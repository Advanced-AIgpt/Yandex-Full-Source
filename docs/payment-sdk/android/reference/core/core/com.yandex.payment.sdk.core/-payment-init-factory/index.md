//[core](../../../index.md)/[com.yandex.payment.sdk.core](../index.md)/[PaymentInitFactory](index.md)

# PaymentInitFactory

[core]\
class [PaymentInitFactory](index.md)(**context**: Context, **environment**: [PaymentSdkEnvironment](../../com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md), **consoleLoggingMode**: [ConsoleLoggingMode](../../com.yandex.payment.sdk.core.data/-console-logging-mode/index.md), **metricaInitMode**: [MetricaInitMode](../../com.yandex.payment.sdk.core.data/-metrica-init-mode/index.md))

Базовая фабрика для инициализации, используется для задания окружения [PaymentSdkEnvironment](../../com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md) и режима логирования [ConsoleLoggingMode](../../com.yandex.payment.sdk.core.data/-console-logging-mode/index.md).

## Constructors

| | |
|---|---|
| [PaymentInitFactory](-payment-init-factory.md) | [core]<br>fun [PaymentInitFactory](-payment-init-factory.md)(context: Context, environment: [PaymentSdkEnvironment](../../com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md) = PaymentSdkEnvironment.PRODUCTION, consoleLoggingMode: [ConsoleLoggingMode](../../com.yandex.payment.sdk.core.data/-console-logging-mode/index.md) = ConsoleLoggingMode.AUTOMATIC, metricaInitMode: [MetricaInitMode](../../com.yandex.payment.sdk.core.data/-metrica-init-mode/index.md) = MetricaInitMode.CORE) |

## Functions

| Name | Summary |
|---|---|
| [createBuilder](create-builder.md) | [core]<br>fun [createBuilder](create-builder.md)(): [PaymentApi.Builder](../-payment-api/-builder/index.md)<br>Создать билдер для [PaymentApi](../-payment-api/index.md). |
| [setupAsync](setup-async.md) | [core]<br>fun [setupAsync](setup-async.md)(uuid: String?, completion: () -> Unit? = null)<br>Установить окружение для экспериментов PaymentSDK. |
