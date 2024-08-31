# Quick start или как быстро чем-нибудь заплатить

## Что нужно иметь
* у вас должен быть `service token` вашего сервиса в Trust
* в приложении должен быть залогин через тестовый Паспорт
* у тестового пользователя должна быть привязана хоть одна карта
* создайте корзину и получите `purchase_token`

Теперь разберём пример `mobile/payment-sdk/ios/examples/UsingPrebuiltUI` и заплатим уже существующей банковской карточкой пользователя.

## Основные объекты
* Основная точка входа в апи - [PaymentApi](../core/PaymentApi.md)
* Объект для управления конкретным платежом, получается после старта платежа - [PaymentInstance](../core/PaymentInstance.md)
* Интерфейс вью для ввода CVV/CVC - [CvnInput](../ui/CvnInput.md)

## Шаг 1 - Инициализация Api
Всё происходит в методе `initPaymentSdk()`
```swift
    private func initPaymentSdk() {
        let credentials = PaymentCredentials.fromCredentialsJson()
        let factory = PaymentFactory.setPaymentKitMode(.debug).initialize()
        paymentApi = factory.makePaymentApiBuilder(payer: Payment.Payer(oauthToken: credentials?.oauthToken, email: credentials?.email, uid: credentials?.uid), merchant: Payment.Merchant(serviceToken: credentials?.serviceToken ?? "", localizedName: ""), callbacks: self)
        // Передайте внутрь false - тогда CVV появится только если это сконфигурировано на бэке или его запросит антифрод
            .forceCVV(true)
            .build()
        self.credentials = credentials
    }
```
Важные моменты:
* Наш скрипт для сэмплов получает OAuth-токен сразу со скоупом платежей, вам в вашем приложении нужно будет получить скоуп в платежей в паспорте для вашего залогина. Подробности как это делать [есть тут](../oauth.md).
* Флаг forceCVV нужен для отладки и передаёт на наш фронтбэк специальный заголовок, при котором для всех карт будет запрошен CVV. Попробуйте с этим флагом и без него.

## Шаг 2 - Список методов оплат
Далее загружаем доступные методы оплаты используя `PaymentApi.paymentMethods` и ищем среди них первую же банковскую карту, см. метод `loadMethods()`

## Шаг 3 - Старт платежа и оплата
На этом этапе мы стартуем платёж `PaymentApi.startPayment`, и, если всё ок, тут же начинаем процесс оплаты банковской картой.
Здесь также проверяем необходимость запроса CVV `PaymentInstance.shouldShowCvv` и создаём вью для этого, если нужно.
```swift
    let cvnInput = uiFactory.createCvnInputView()
    cvnInput.setPaymentApi(api)
    cvnInput.setCardPaymentSystem(paymentSystem)
    // Здесь мы сразу при получении корректного CVV продолжаем платёж, но можно завязать на нажатие кнопки и тд, как сделано в самом PaymentSDK
    cvnInput.setOnReadyCallback { [weak self] ready in
        if ready {
            cvnInput.provideCvn()
            cvnInput.reset()
            cvnInput.isHidden = true
            self?.stackView.removeArrangedSubview(cvnInput)
        }
    }
    
    stackView.addArrangedSubview(cvnInput)
```
`CvnInput` нужно обязательно соединить с `PaymentApi`, а также проставить ему платёжную систему выбранной карты - это нужно для уточнения правил валидирования CVV.
В нашем примере мы будем запускать посылку CVV сразу как при успешной валидации.
