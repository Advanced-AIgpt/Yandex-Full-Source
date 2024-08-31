# Работа с API привязки банковских карт

Существующее API для работы можно посмотреть в интерфейсе `BindApi` [Android](android/reference/core/core/com.yandex.payment.sdk.core/-payment-api/-bind-api/index.md), [iOS](ios/reference/core/BindApi.md).

## Привязка

### Схемы
Сейчас существует две схемы привязки:
1. Привязка через апи V2, в данном случае проверочный платёж будет проходить через сервис, заданный в `Merchant` ([Android](android/reference/core/core/com.yandex.payment.sdk.core.data/-merchant/index.md), [iOS](ios/reference/core/Payment_Merchant.md)) при построении `PaymentApi`. Также вам может потребоваться отобразить 3ds страничку. Соответствующий метод `bindCardWithVerify` ([Android](android/reference/core/core/com.yandex.payment.sdk.core/-payment-api/-bind-api/bind-card-with-verify.md), [iOS](ios/reference/core/BindApi.md#bindcardwithverifycompletion​)).

1. Синхронная через ручку Траста `bind_card`, в этом случае проверочный платеж будет на сервисе Паспорта, а PaymentSDK сможет получить только финальный результат операции. Соответствующий метод `bindCardWithoutVerify` ([Android](android/reference/core/core/com.yandex.payment.sdk.core/-payment-api/-bind-api/bind-card-without-verify.md), [iOS](ios/reference/core/BindApi.md#bindcardwithoutverifycompletion​))
{% note warning %}

Этот метод устарел и через какое-то время будет убран. Также он работает только для ограниченного специальным вайтлистом числа сервисов.

{% endnote %}

### Процесс
Для привязки создайте объект `PaymentApi` и вызовите метод `bindCardWithVerify`:
```kotlin
val paymentApi = ...
paymentApi.Bind.bindCardWithVerify(completion)
```
```swift
let paymentApi = ...
paymentApi.Bind.bindCardWithVerify(completion: completion)
```

Вы получите вызов коллбека `newCardDataRequested` в заданные `PaymentCallbacks`([Android](android/reference/core/core/com.yandex.payment.sdk.core.data/-payment-callbacks/index.md), [iOS](ios/reference/core/PaymentCallbacks.md)) при создании апи. Для ввода карточных данных существует Prebuilt UI компонент `CardInput`, с которым нужно взаимодействовать по [схеме](scheme.md).
Далее в случае работы через схему V2 вы можете получить запрос на ввод 3ds, который нужно отобразить пользователю.
Для упрощения интеграции вы можете использовать `BindCardMediator` ([Android](android/reference/datasource/datasource/com.yandex.payment.sdk.datasource.bind/-bind-card-mediator/index.md), [iOS](ios/reference/mediators/BindCardMediator.md)) и расширить его при необходимости.

## Отвязка карт
Просто вызовите метод `unbindCard`, в `completion` вы получите результат.

## Верификация карт
{% note warning %}

Работает только через 3ds, random_amt сейчас не поддержан в PaymentSDK.

{% endnote %}

Используйте метод `verifyCard`. Возможно потребуется отобразить страницу 3ds.