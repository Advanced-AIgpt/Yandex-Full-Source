//[core](../../index.md)/[com.yandex.payment.sdk.core.data](index.md)

# Package com.yandex.payment.sdk.core.data

## Types

| Name | Summary |
|---|---|
| [Acquirer](-acquirer/index.md) | [core]<br>enum [Acquirer](-acquirer/index.md) : Enum<[Acquirer](-acquirer/index.md)> <br>Обработчик платежа. |
| [AppInfo](-app-info/index.md) | [core]<br>data class [AppInfo](-app-info/index.md)(**psuid**: String?, **tsid**: String?, **appid**: String?) : Parcelable<br>Информация о турбоаппе ПП/Браузера. |
| [BankAppInfo](-bank-app-info/index.md) | [core]<br>sealed class [BankAppInfo](-bank-app-info/index.md) |
| [BankName](-bank-name/index.md) | [core]<br>enum [BankName](-bank-name/index.md) : Enum<[BankName](-bank-name/index.md)> <br>Перечисление банков. |
| [BoundCard](-bound-card/index.md) | [core]<br>data class [BoundCard](-bound-card/index.md)(**cardId**: String, **rrn**: String?) : Parcelable<br>Информация о привязке/верификации карты. |
| [BrowserCard](-browser-card/index.md) | [core]<br>data class [BrowserCard](-browser-card/index.md)(**number**: String, **expirationMonth**: String, **expirationYear**: String) : Parcelable<br>Тип для передачи карт из Браузера/ПП. |
| [CardBinValidationConfig](-card-bin-validation-config/index.md) | [core]<br>class [CardBinValidationConfig](-card-bin-validation-config/index.md) : Parcelable<br>Конфиг для валидации BIN номеров банковских карт. |
| [CardExpirationDateValidationConfig](-card-expiration-date-validation-config/index.md) | [core]<br>class [CardExpirationDateValidationConfig](-card-expiration-date-validation-config/index.md) : Parcelable<br>Конфиг для валидации expiration date банковских карт. |
| [CardId](-card-id/index.md) | [core]<br>class [CardId](-card-id/index.md)<br>Тип идентификатора карты. |
| [CardPaymentSystem](-card-payment-system/index.md) | [core]<br>enum [CardPaymentSystem](-card-payment-system/index.md) : Enum<[CardPaymentSystem](-card-payment-system/index.md)> <br>Перечисление платежных систем. |
| [CardValidationConfig](-card-validation-config/index.md) | [core]<br>class [CardValidationConfig](-card-validation-config/index.md)(**binConfig**: [CardBinValidationConfig](-card-bin-validation-config/index.md), **expirationDateConfig**: [CardExpirationDateValidationConfig](-card-expiration-date-validation-config/index.md)) : Parcelable<br>Общий конфиг валидации карт. |
| [ConsoleLoggingMode](-console-logging-mode/index.md) | [core]<br>enum [ConsoleLoggingMode](-console-logging-mode/index.md) : Enum<[ConsoleLoggingMode](-console-logging-mode/index.md)> , Parcelable<br>Режим логирования PaymentSDK в логкат. |
| [FamilyInfo](-family-info/index.md) | [core]<br>class [FamilyInfo](-family-info/index.md)(**familyAdminUid**: String, **familyId**: String, **expenses**: Int, **limit**: Int, **currency**: String, **frame**: String, **isUnlimited**: Boolean) : Parcelable<br>Информация о владельце карты, доступной пользователю Подробнее [по ссылке](https://wiki.yandex-team.ru/users/amosov-f/familypay/#variantformataotvetaruchkilpmvtraste) |
| [GooglePayActivityResultStorage](-google-pay-activity-result-storage/index.md) | [core]<br>interface [GooglePayActivityResultStorage](-google-pay-activity-result-storage/index.md)<br>Интерфейс для передачи результата активити работы GooglePay. |
| [GooglePayAllowedCardNetworks](-google-pay-allowed-card-networks/index.md) | [core]<br>class [GooglePayAllowedCardNetworks](-google-pay-allowed-card-networks/index.md) : Parcelable<br>Доступные платежные системы Google Pay. |
| [GooglePayData](-google-pay-data/index.md) | [core]<br>sealed class [GooglePayData](-google-pay-data/index.md) : Parcelable<br>Sealed классы для настроек Google Pay. |
| [GooglePayHandler](-google-pay-handler/index.md) | [core]<br>interface [GooglePayHandler](-google-pay-handler/index.md)<br>Интерфейс обработчика GooglePay. |
| [GooglePayToken](-google-pay-token/index.md) | [core]<br>class [GooglePayToken](-google-pay-token/index.md)(**token**: String) : Parcelable<br>Результирующий токен от GooglePay. |
| [GooglePayTrustMethod](-google-pay-trust-method/index.md) | [core]<br>data class [GooglePayTrustMethod](-google-pay-trust-method/index.md)(**paymentMethodId**: String, **paymentId**: String)<br>Результат привязки токена GooglePay в Трасте. |
| [Merchant](-merchant/index.md) | [core]<br>data class [Merchant](-merchant/index.md)(**serviceToken**: String) : Parcelable<br>Данные о сервисе. |
| [MerchantAddress](-merchant-address/index.md) | [core]<br>class [MerchantAddress](-merchant-address/index.md)(**country**: String, **city**: String, **street**: String, **home**: String, **zip**: String) : Parcelable<br>Адрес мерчанта. |
| [MerchantInfo](-merchant-info/index.md) | [core]<br>class [MerchantInfo](-merchant-info/index.md)(**name**: String, **scheduleText**: String, **ogrn**: String, **merchantAddress**: [MerchantAddress](-merchant-address/index.md)?) : Parcelable<br>Данные о мерчанте. |
| [MetricaInitMode](-metrica-init-mode/index.md) | [core]<br>enum [MetricaInitMode](-metrica-init-mode/index.md) : Enum<[MetricaInitMode](-metrica-init-mode/index.md)> <br>Способ работы с метрикой. |
| [OrderDetails](-order-details/index.md) | [core]<br>sealed class [OrderDetails](-order-details/index.md) : Parcelable<br>Параметры заказа для GooglePay. |
| [OrderInfo](-order-info/index.md) | [core]<br>data class [OrderInfo](-order-info/index.md)(**orderTag**: String?, **orderDetails**: [OrderDetails](-order-details/index.md)?) : Parcelable<br>Информация о заказе. |
| [Payer](-payer/index.md) | [core]<br>data class [Payer](-payer/index.md)(**oauthToken**: String?, **email**: String?, **uid**: String?, **firstName**: String?, **lastName**: String?, **phone**: String?) : Parcelable<br>Информация о пользователе. |
| [PaymentCallbacks](-payment-callbacks/index.md) | [core]<br>interface [PaymentCallbacks](-payment-callbacks/index.md)<br>Основные коллбеки [com.yandex.payment.sdk.core.PaymentApi](../com.yandex.payment.sdk.core/-payment-api/index.md) |
| [PaymentKitError](-payment-kit-error/index.md) | [core]<br>data class [PaymentKitError](-payment-kit-error/index.md)(**kind**: [PaymentKitError.Kind](-payment-kit-error/-kind/index.md), **trigger**: [PaymentKitError.Trigger](-payment-kit-error/-trigger/index.md), **code**: Int?, **status**: String?, **message**: String) : Throwable, Parcelable<br>Класс ошибок PaymentSDK. |
| [PaymentMethod](-payment-method/index.md) | [core]<br>sealed class [PaymentMethod](-payment-method/index.md)<br>Платежные методы. |
| [PaymentMethodsFilter](-payment-methods-filter/index.md) | [core]<br>data class [PaymentMethodsFilter](-payment-methods-filter/index.md)(**isStoredCardAvailable**: Boolean, **isGooglePayAvailable**: Boolean, **isSBPAvailable**: Boolean, **isYandexBankAccountAvailable**: Boolean) : Parcelable<br>Фильтр способов оплаты в списке. |
| [PaymentPollingResult](-payment-polling-result/index.md) | [core]<br>enum [PaymentPollingResult](-payment-polling-result/index.md) : Enum<[PaymentPollingResult](-payment-polling-result/index.md)> <br>Результат поллинга платежа. |
| [PaymentSdkEnvironment](-payment-sdk-environment/index.md) | [core]<br>enum [PaymentSdkEnvironment](-payment-sdk-environment/index.md) : Enum<[PaymentSdkEnvironment](-payment-sdk-environment/index.md)> , Parcelable<br>Окружение PaymentSDK. |
| [PaymentSettings](-payment-settings/index.md) | [core]<br>class [PaymentSettings](-payment-settings/index.md)(**total**: String, **currency**: String, **licenseURL**: Uri?, **acquirer**: [Acquirer](-acquirer/index.md)?, **environment**: String, **merchantInfo**: [MerchantInfo](-merchant-info/index.md)?, **payMethodMarkup**: [PaymethodMarkup](-paymethod-markup/index.md)?, **creditFormUrl**: String?) : Parcelable<br>Подробная информация о платеже. |
| [PaymentToken](-payment-token/index.md) | [core]<br>data class [PaymentToken](-payment-token/index.md)(**token**: String) : Parcelable<br>Токен корзины для оплаты. |
| [PaymethodMarkup](-paymethod-markup/index.md) | [core]<br>class [PaymethodMarkup](-paymethod-markup/index.md)(**card**: String?) : Parcelable<br>Разметка платежа из Траста. |
| [Result](-result/index.md) | [core]<br>sealed class [Result](-result/index.md)<[T](-result/index.md)><br>Класс для хранения результата. |
| [SbpHandler](-sbp-handler/index.md) | [core]<br>interface [SbpHandler](-sbp-handler/index.md)<br>Интерфейс обработчика для Системы Быстрых Платежей. |
