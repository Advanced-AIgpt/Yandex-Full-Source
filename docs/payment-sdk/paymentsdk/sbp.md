# Как быстро запустить СБП через PaymentSDK

Необходимо:
* Примерно понимать как устроены платежи в Яндексе
* Быть интегрированным с Трастом
* Иметь подключенный и настроенный терминал с СБП, заказ новых терминалов [тут](https://st.yandex-team.ru/createTicket?queue=GMSP&_form=84425)

Оплата через СБП пройдёт только если у пользователя установлены приложения банков.
При желании можно заранее проверить это с помощью вызовов `PaymentKit.getInstalledBankInfo`, а также через Core API на [Android](../android/reference/core/core/com.yandex.payment.sdk.core/-payment-api/get-installed-bank-info.md), и на [iOS](../ios/reference/core/PaymentApi.md#getinstalledbankinfocompletion​​).
К сожалению метод не срабатывает на некоторых кривых приложениях банков на Android, а также на приложениях со схемами не из plist в iOS, так что рекомендуется всегда показывать способ оплаты СБП, если он доступен для данного заказа.

Есть вопросы? Приходите в [чат поддержки](https://t.me/joinchat/ECJJh1Cv6Jj2g-r7XpCgwg) ([резервный чат в Q](https://q.yandex-team.ru/#/join/7a116050-4260-4a9c-82c6-fafceaf3a18d))

## Android

### Подключение
```
maven { url 'http://artifactory.yandex.net/yandex_mobile_releases/' } - для релизных версий
maven { url 'http://artifactory.yandex.net/artifactory/yandex_mobile_snapshots/' } - для снепшотов
[...]
implementation com.yandex.paymentsdk:paymentsdk:<version> 
```
### Вызов
```kotlin
val merchant = Merchant("service_token")
val payer = Payer("oauth", "test@ya.ru", "uid", null, null, null)
val settings =  AdditionalSettings.Builder().setExchangeOauthToken(true).build()
val paymentFactory = PaymentFactory.Builder()
    .setContext(applicationContext)
    .setEnvironment(environment = PaymentSdkEnvironment.TESTING /* для прода .PRODUCTION */)
    .build()
val paymentKit =  paymentFactory.createPaymentKit(
    applicationContext,
    payer,
    merchant,
    additionalSettings = settings
)
paymentKit.updateTheme(DefaultTheme.LIGHT) // можно установить тёмную или светлую тему, по дефолту светлая

val paymentToken = PaymentToken("token")
val intent = createPaymentIntent<PaymentActivity>(
    PaymentToken(token),
    PaymentOption.sbp()) // Передача укажет PaymentSDK сразу запустить оплату через СБП без отображения списка методов
startActivityForResult(
    intent,
    LAUNCH_PAYMENT_ACTIVITY
) // открывается экран выбора способа оплаты с последующей оплатой

override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    super.onActivityResult(requestCode, resultCode, data)
    if (requestCode == LAUNCH_PAYMENT_ACTIVITY) {
        if (resultCode == Activity.RESULT_OK) {
            // успешная оплата
        }
    }
}
```
{% note warning %}

PaymentKit - не синглтон, его нужно сохранить

{% endnote %}

## iOS

### Подключение
```
source 'https://bitbucket.browser.yandex-team.ru/scm/mcp/mobile-cocoa-pod-specs.git'
[...]
pod 'YandexPaymentSDK', '<version>'
```

### Вызов

{% note warning %}

Не забудьте про обмен скоупов OAuth-токена, см [сюда](../oauth.md)

{% endnote %}

```swift
let factory = PaymentFactory
    .setPaymentKitMode(environment.paymentSdkMode)
    .setMetricaMode(.depended)
    .setConsoleLoggingMode(.enabled)
    .initialize()

let merchant = Payment.Merchant(serviceToken: "service_token")
let payer = Payment.Payer(oauthToken: "oauth_token", email: "test@ya.ru", uid: "1234")

let paymentKit = factory.makePaymentKit(
    payer: payer,
    merchant: YandexPaymentSDK.Payment.Merchant(
        serviceToken: "token",
        localizedName: "for_apple_pay_only"
    ),
    additionalSettings: additionalSettings.build()
)

let presentingController = UIViewController()
let token = Payment.Token(token: "token")
let processCompletion = Callbacks.ProcessCompletion(didFinishWithStatus: { _ in }, wasDismissed: {}) // обработка статуса завершения платежа, закрытия диалога
paymentKit.presentPaymentController(for: token,
                                    selectedOption: PaymentOption.sbp() // Передача укажет PaymentSDK сразу запустить оплату через СБП без отображения списка методов
                                    in: presentingController,
                                    animated: true,
                                    processCompletion: processCompletion,
                                    completion: nil) // открывается экран выбора способа оплаты с последующей оплатой
```
{% note warning %}

PaymentKit - не синглтон, его нужно сохранить

{% endnote %}

### Обнаружение банковских приложений на телефоне
Чтобы PaymentSDK смогло определить установленные банки на телефоне через `UIApplication.canOpenURL` необходимо добавить схемы приложений банков в plist.
Решение неидеальное, так как с iOS 15 количество таких схем ограничено для приложений, НСПК ничегос этим сделать не смогло.
Можно занести в plist топовые банки, а остальные PaymentSDK начиная с версии `3.11.0` будет показывать в общем списке всех банков.

Сейчас для получения списка PaymentSDK ходит в наш бэкенд, который проксирует вызов в ручку [https://qr.nspk.ru/proxyapp/c2bmembers.json](https://qr.nspk.ru/proxyapp/c2bmembers.json).

Пример plist'а [Заправок](https://bb.yandex-team.ru/projects/TANKER/repos/tanker-ios-navigator/browse/GasStations/GasStations/Info.plist#71-128)

{% cut "Схемы топа банковских приложений" %}
```
<string>bank100000000111</string>
<string>bank100000000004</string>
<string>bank100000000005</string>
<string>bank100000000008</string>
<string>bank100000000007</string>
<string>bank100000000015</string>
<string>bank100000000001</string>
<string>bank100000000010</string>
<string>bank100000000013</string>
<string>bank100000000012</string>
<string>bank100000000020</string>
<string>bank100000000025</string>
<string>bank100000000030</string>
<string>bank100000000003</string>
<string>bank100000000043</string>
<string>bank100000000028</string>
<string>bank100000000086</string>
<string>bank100000000011</string>
<string>bank100000000044</string>
<string>bank100000000054</string>
<string>bank100000000049</string>
<string>bank100000000500</string>
<string>bank100000000095</string>
<string>bank100000000900</string>
<string>bank100000000056</string>
<string>bank100000000053</string>
<string>bank100000000121</string>
<string>bank100000000082</string>
<string>bank100000000127</string>
<string>bank100000000017</string>
<string>bank100000000087</string>
<string>bank100000000052</string>
<string>bank100000000006</string>
<string>bank100000000098</string>
<string>bank100000000092</string>
<string>bank100000000229</string>
<string>bank100000000027</string>
<string>bank100000000080</string>
<string>bank100000000122</string>
<string>bank100000000124</string>
<string>bank100000000118</string>
<string>bank100000000159</string>
<string>bank100000000146</string>
<string>bank100000000069</string>
<string>bank100000000140</string>
<string>bank100000000047</string>
<string>bank100000000099</string>
<string>bank100000000135</string>
<string>bank100000000139</string>
<string>bank100000000166</string>
<string>bank100000000172</string>
<string>bank100000000187</string>
<string>bank100000000022</string>
<string>bank100000000029</string>
<string>bank100000000050</string>
<string>bank100000000065</string>
<string>bank100000000177</string>
<string>bank100000000084</string>
<string>bank100000000088</string>
```
{% endcut %}

## Тестирование

К сожалению, в тестовом окружении протестировать полностью оплату СБП не получится. Единственный вариант - тест на проде, для чего вам придётся открыть сначала этот метод оплаты только на ваших тестировщиков и возвращать деньги через рефанды.