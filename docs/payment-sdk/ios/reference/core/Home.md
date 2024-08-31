# Types

  - [PaymentApiBuilder](PaymentApiBuilder.md):
    Билдер для создания объекта `PaymentApi`.
  - [PaymentFactory](PaymentFactory.md):
    Фабрика для инициализации.
  - [Order.Builder](Order_Builder.md)
  - [Metrica](Metrica.md)
  - [PaymentSdkResources](PaymentSdkResources.md):
    Получение различных общих ресурсов.
  - [PaymentKitMode](PaymentKitMode.md)
  - [ConsoleLoggingMode](ConsoleLoggingMode.md):
    Режим логирования PaymentSDK.
  - [AuthChallengeDisposition](AuthChallengeDisposition.md):
    Типы проверок при установке соединения в методах делегатов URLSession и WKWebView
    urlSession(*:, didReceive challenge:completionHandler:)
    webView(*:, didReceive challenge: completionHandler:)
  - [Payment](Payment.md):
    Общий enum-скоуп для данных, связанных с платежами
  - [Payment.Status](Payment_Status.md)
  - [Payment.Option.Ids](Payment_Option_Ids.md)
  - [Payment.Card](Payment_Card.md):
    Информация о банковской карте.
  - [Payment.Card.PaymentSystem](Payment_Card_PaymentSystem.md):
    Платёжная система.
  - [Payment.BankName](Payment_BankName.md):
    Название банка.
  - [Payment.Method](Payment_Method.md):
    Метод оплаты.
  - [Payment.RegionId](Payment_RegionId.md):
    Регион по геобазе.
  - [Payment.Settings.Acquirer](Payment_Settings_Acquirer.md):
    Обработчик платежа. Только для Яндекс.Оплат.
  - [PaymentKitError.Kind](PaymentKitError_Kind.md)
  - [PaymentKitError.Trigger](PaymentKitError_Trigger.md)
  - [PaymentPollingResult](PaymentPollingResult.md):
    Результат поллинга платежа. Для всех методов кроме Тинькофф кредита это всегда будет `success`.
    Для кредитов возможны оба статуса, в зависимости от одобрения кредита.
  - [AuthEnvironment](AuthEnvironment.md)
  - [Metrica.Mode](Metrica_Mode.md)
  - [BoundApplePayToken](BoundApplePayToken.md):
    Привязанный в Trust ApplePay токен.
  - [BoundCard](BoundCard.md):
    Информация о привязке/верификации карты.
  - [Order](Order.md):
    Параметры заказа для ApplePay.
  - [Order.Tag](Order_Tag.md):
    Тэг заказа.
  - [Order.CurrencyAmount](Order_CurrencyAmount.md):
    Сумма заказа.
  - [Order.PaymentSheet](Order_PaymentSheet.md):
    Список позиций для отображения в контроллере ApplePay.
  - [Payment.Payer](Payment_Payer.md):
    Информация о пользователе.
  - [Payment.Merchant](Payment_Merchant.md):
    Данные о сервисе.
  - [Payment.Token](Payment_Token.md):
    Токен корзины для оплаты.
    В случае Яндекс.Оплат это отдельный токен, не совпадает с purchase token Траста.
    В остальных случаях эквивалентен purchase token.
  - [Payment.FamilyInfo](Payment_FamilyInfo.md):
    Информация о владельце карты, доступной пользователю.
    Подробнее [по ссылке](https://wiki.yandex-team.ru/users/amosov-f/familypay/#variantformataotvetaruchkilpmvtraste)
  - [Payment.PartnerInfo](Payment_PartnerInfo.md)
  - [Payment.Option](Payment_Option.md)
  - [Payment.BankAppInfo](Payment_BankAppInfo.md):
    Информация о банковском приложении.
  - [Payment.BankAppLaunchInfo](Payment_BankAppLaunchInfo.md):
    Информация о банковском приложении.
  - [Payment.Card.Id](Payment_Card_Id.md):
    Идентификатор банковской карты в diehard.
  - [Payment.ApplePay](Payment_ApplePay.md):
    Настройки ApplePay.
  - [Payment.MethodsFilter](Payment_MethodsFilter.md):
    Фильтр методов оплаты в списке.
  - [Payment.AppInfo](Payment_AppInfo.md):
    Информация о турбоаппе ПП/Браузера.
  - [Payment.Settings](Payment_Settings.md):
    Подробная информация о платеже.
  - [Payment.Settings.MerchantInfo](Payment_Settings_MerchantInfo.md):
    Данные о мерчанте. Только для Яндекс.Оплат.
  - [Payment.Settings.PayMethodMarkup](Payment_Settings_PayMethodMarkup.md):
    Разметка платежа из Траста.
  - [PaymentKitError](PaymentKitError.md):
    Класс ошибок PaymentSDK
  - [AuthCredentials](AuthCredentials.md)

# Protocols

  - [ApplePayApi](ApplePayApi.md):
    Протокол для работы с ApplePay. Содержит вспомогательные методы, не работающие с самими платежами в Трасте.
  - [ApplePayDelegate](ApplePayDelegate.md):
    Делегат для работы с контроллером ApplePay.
  - [BindApi](BindApi.md):
    Интерфейс для работы с банковскими картами.
  - [PaymentApi](PaymentApi.md):
    Главный протокол для работы с API - позволяет работать с платежами, привязками банковских карт и ApplePay. Создаётся
    через `PaymentApiBuilder`.
  - [PaymentCallbacks](PaymentCallbacks.md):
    Основные коллбеки `PaymentApi`.
  - [PaymentInstance](PaymentInstance.md):
    Интерфейс для работы с оплатой. Создаётся после успешного `PaymentApi.startPayment`.
    **Важно:** в один момент времени может идти работа только с одним платежом, если нужно провести
    ещё одну оплату - нужно дождаться завершения предыдущей или сбросить её `cancel`.
  - [PaymentKitAuthExchanger](PaymentKitAuthExchanger.md):
    Протокол обмена скоупа токенов в Паспорте.

# Global Typealiases

  - [SbpHandler](SbpHandler.md):
    Алиас для обработчика СБП.
  - [PaymentKitResult](PaymentKitResult.md)
  - [ExternalResult](ExternalResult.md)

# Extensions

  - [Swift.Result](Swift_Result.md)
