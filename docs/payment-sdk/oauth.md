# Обмен OAuth токенов для получения скоупа платежей

Для авторизации в СДК пользовательские токены должны быть в скоупе `payments:all`. В случае, если не делать проверку скоупа, любое стороннее приложение сможет дергать API оплаты под аккаунтом пользователя (в том числе получать списки карт для оплаты), например, запросив у него доступ только на чтение email. Также, если приложение–хост где-то залогирует токен пользователя, то по этому токену можно будет получить доступ к данным пользователя как внутри этого приложения, так и внутри СДК.

## Android

Для включения функционала обмена скоупа в `Android`  вам нужно включить настройку `exchangeOauthToken` в `AdditionalSettings` если вы используете диалог PaymentSDK целиком (работаете через `PaymentKit`), или в [PaymentApi#Builder](android/reference/core/core/com.yandex.payment.sdk.core/-payment-api/-builder/index.md) если вы работаете с [PaymentApi](android/reference/core/core/com.yandex.payment.sdk.core/-payment-api/index.md) , обмен скоупа произойдёт автоматически внутри СДК.

## iOS

В iOS внутри СДК нет автоматического обмена скоупов для токена, необходимо реализовать протокол [PaymentKitAuthExchanger](ios/reference/core/PaymentKitAuthExchanger.md), обменять токен в Паспорте, а затем передать в СДК новый токен.

{% cut "Пример реализации" %}
```
    enum PaymentSDKAuthExchangerError: Error {
            case uidToIntFailed
            case tokenExchangeFailed
    }
    
    func exchangeOauthToken(uid: String,
                            originalOauthToken: String,
                            authEnvironment: AuthEnvironment,
                            completion: @escaping (ExternalResult<String>) -> Void) {
        guard let uidAsInt = Int(uid) else {
            completion(.failure(PaymentSDKAuthExchangerError.uidToIntFailed))
            return
        }
        let uidAsNumber = NSNumber(value: uidAsInt)
        let sdkCredentials = authEnvironment.getSdkCredentials()
        let credentials = YALClientIdentifier(clientID: sdkCredentials.clientID,
                                              clientSecret: sdkCredentials.clientSecret)
        AccountManager.shared
            .obtainTokenForAccount(withUid: uidAsNumber,
                                   credentials: credentials) { (account, error) in
                if let authToken = account?.token {
                    completion(.success(authToken))
                } else if let error = error {
                    completion(.failure(error))
                } else {
                    completion(.failure(PaymentSDKAuthExchangerError.tokenExchangeFailed))
                }
            }
    }
```
{% endcut %}

Или можно посмотреть в коде [Заправок](https://bb.yandex-team.ru/projects/TANKER/repos/tanker-ios-navigator/browse/GasStations/GasStations/Controllers/Authorization/AccountPaymentKitAuthExchanger.swift)

Релизацию проставить в [PaymentFactory.setAuthExchanger](ios/reference/core/PaymentFactory.md)
```swift
let authExchanger = PaymentKitAuthExchangerImpl()
let factory = PaymentFactory
    .setPaymentKitMode(environment.paymentSdkMode)
    .setMetricaMode(.depended)
    .setConsoleLoggingMode(.enabled)
    .setAuthExchanger(handler: authExchanger)
    .initialize()
```

## Возможные проблемы

Если после успешного обмена токенов diehard всё равно возвращает ошибки со статусом 401 (`invalid scope`), нужно убедиться, что ваш сервис в АМ имеет признак `client_is_yandex = 1` ([пример обращения](https://st.yandex-team.ru/OAUTHREG-523#62948d91ac6573518d253e1f))
