# How To Dev

## Подключение
iOS:
Sources - [https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/ios/YandexPaymentSDK](https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/ios/YandexPaymentSDK)
CocoaPods:
```
source 'https://bitbucket.browser.yandex-team.ru/scm/mcp/mobile-cocoa-pod-specs.git'
[...]
pod 'YandexPaymentSDK', '<version>'
```
Android:
Sources - [https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/android](https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/android)
Gradle:
```
maven { url 'http://artifactory.yandex.net/yandex_mobile_releases/' } - для релизных версий
maven { url 'http://artifactory.yandex.net/artifactory/yandex_mobile_snapshots/' } - для снепшотов
[...]
implementation com.yandex.paymentsdk:paymentsdk:<version> 
```

## Функциональность PaymentSDK

### Оплата

В данном кейсе пользователь может выбрать способ оплаты и сразу оплатить им заказ. Способы оплаты, доступные для пользователя и сервиса, в котором осуществляется оплата, приходят из Траста. Но их также можно фильтровать на клиенте.

{% note warning %}

Если вы видите бесконечный лоадинг, но при этом вам пришел респонс от бэка со способами оплаты, скорей всего вы не сохранили переменную paymentKit. PaymentKit  - не синглтон, его нужно сохранить локально в классе, где вы его создаёте, иначе он будет уничтожен за пределами скоупа.

{% endnote %}


{% cut "Screenshots iOS" %}

![Список](_assets/oplata_ios_1.png "Список" =270x480)
![Загрузка](_assets/oplata_ios_2.png "Загрузка" =270x480)
![Успех](_assets/oplata_ios_3.png "Успех" =270x480)
![Ошибка](_assets/oplata_ios_4.png "Ошибка" =270x480)

{% endcut %}

{% cut "Screenshots Android" %}

![Список](_assets/oplata_android_1.png "Список" =270x480)
![Загрузка](_assets/oplata_android_2.png "Загрузка" =270x480)
![Успех](_assets/oplata_android_3.png "Успех" =270x480)
![Ошибка](_assets/oplata_android_4.png "Ошибка" =270x480)

{% endcut %}

{% cut "Code snippet iOS" %}
```swift
// настройка библиотеки PaymentSDK, можно делать до старта формы оплаты при запуске приложения или в любом другом месте, где вы инициализируете внешние библиотеки
let factory = PaymentFactory
    .setConsoleLoggingMode(.enabled) /* включает логирование в консоль событий, которые происходят во время работы сдк */
    .setPaymentKitMode(.debug) /* для прода .release */
    .initialize()
        
let merchant = Payment.Merchant(serviceToken: "service_token")
let payer = Payment.Payer(oauthToken: "oauth_token", email: "test@ya.ru", uid: "1234")
var settings = AdditionalSettings()
var filter = AdditionalSettings.PaymentMethodsFilter() // внешний фильтр, который позволяет отключить с клиента определённые способы оплаты, которые пришли с бэка, если отключить все, будет сразу показан UI оплаты новой карты
filter.isSBPAvailable = false // например, СБП
settings.paymentMethodsFilter = filter
let paymentKit = factory.makePaymentKit(
                         payer: payer,
                         merchant: merchant,
                         additionalSettings: settings
                      )
paymentKit.applyUITheme(.light) // можно установить тёмную или светлую тему, по дефолту светлая

// NOTE: paymentKit - не синглтон, его нужно сохранить локально в классе, где вы его создаёте,
// иначе он будет уничтожен за пределами скоупа
        
let presentingController = UIViewController()
let token = Payment.Token(token: "token")
let processCompletion = Callbacks.ProcessCompletion(didFinishWithStatus: { _ in }, wasDismissed: {}) // обработка статуса завершения платежа, закрытия диалога
paymentKit.presentPaymentController(for: token,
                                    in: presentingController,
                                    animated: true,
                                    processCompletion: processCompletion,
                                    completion: nil) // открывается экран выбора способа оплаты с последующей оплатой
```
{% endcut %}

{% cut "Code snippet Android" %}
```kotlin
val merchant = Merchant("service_token")
val payer = Payer("oauth", "test@ya.ru", "uid", null, null, null)
val paymentMethodsFilter = PaymentMethodsFilter(
            isStoredCardAvailable = true,
            isGooglePayAvailable = true,
            isSBPAvailable = false
) // внешний фильтр, который позволяет отключить с клиента определённые способы оплаты, которые пришли с бэка, если отключить все, будет сразу показан UI оплаты новой карты, в данном примере будут только карты и Apple/GooglePay
val settings =  AdditionalSettings.Builder()
    .setPaymentMethodsFilter(paymentMethodsFilter)
    .setForceCVV(false)
    .setPersonalInfoConfig(PersonalInfoMode.HIDE)
    .setUseNewCardInputForm(viewBinding.useNewCardInputForm.isChecked)
    .build()

val paymentFactory = PaymentFactory.Builder()
            .setContext(applicationContext)
            .setEnvironment(environment = PaymentSdkEnvironment.TESTING /* для прода .PRODUCTION */)
            .build()
// NOTE: С версии 2.4.0 прямой вызов createPaymentKit без PaymentFactory deprecated
val paymentKit =  paymentFactory.createPaymentKit(applicationContext,
                                   payer,
                                   merchant,
                                   additionalSettings = settings
)
paymentKit.updateTheme(DefaultTheme.LIGHT) // можно установить тёмную или светлую тему, по дефолту светлая

// NOTE: paymentKit - не синглтон, его нужно сохранить

val paymentToken = PaymentToken("token")
startActivityForResult(
    paymentKit.createPaymentIntent<PaymentActivity>(paymentToken),
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
{% endcut %}

### Привязка карты без оплаты

Этот кейс позволяет просто привязать карту к аккаунту пользователя. Существует два разных метода для привязки карты, подробнее о них можно прочитать [тут](#привязка-карты-с-верификацией-aka-v20bindings).
Важно: для этой функциональности не требуется paymentToken , так как осуществляется только привязка, без оплаты чего либо.

{% cut "Screenshots iOS" %}

![Ввод данных](_assets/bind_ios_1.png "Ввод данных" =270x480)
![Некорректный номер](_assets/bind_ios_2.png "Некорректный номер" =270x480)
![Загрузка](_assets/bind_ios_3.png "Загрузка" =270x480)
![Успех](_assets/bind_ios_4.png "Успех" =270x480)

{% endcut %}

{% cut "Screenshots Android" %}

![Ввод данных](_assets/bind_android_1.png "Ввод данных" =270x480)
![Некорректный номер](_assets/bind_android_2.png "Некорректный номер" =270x480)
![Загрузка](_assets/bind_android_3.png "Загрузка" =270x480)
![Успех](_assets/bind_android_4.png "Успех" =270x480)

{% endcut %}

{% cut "Code snippet iOS" %}
```
// создание переменной paymentKit см. в первом кейсе
let callbacks = Callbacks.BindCard { card in
    // данные привязанной карты
}
paymentKit.presentCardBindWithVerifyController(in: presentingController, 
                                               animated: true,
                                               callbacks: callbacks,
                                               processCompletion: processCompletion,
                                               completion: nil) // открывается экран ввода карточных данных
```
{% endcut %}

{% cut "Code snippet Android" %}
```
// создание переменной paymentKit см. в первом кейсе
startActivityForResult(
    createPayment().createBindWithVerifyCardIntent<BindCardActivity>(),
    LAUNCH_BIND_CARD_ACTIVITY
) // открывается экран ввода карточных данных

override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    super.onActivityResult(requestCode, resultCode, data)
    if (requestCode == LAUNCH_BIND_CARD_ACTIVITY) {
        if (resultCode == Activity.RESULT_OK) {
            // данные привязанной карты
            var card = data.getParcelableExtra<BoundCard>(ResultIntentKeys.DATA)
        }
    }
}
```
{% endcut %}

### Выбор способа оплаты

Этот кейс позволяет выбрать способ оплаты и вернуть его, без проведения самой оплаты. Полезен в случае, если в вашем флоу оплаты выбор способа оплаты и сама оплаты разделены какой-то другой логикой. Также в этом кейсе можно отвязать карту, которая была привязана к аккаунту ранее.
В результате возвращается структура `PaymentOption` , которая содержит:
* `id` : идентификатор способа оплаты, для карты это идентификатор из Траста, для СБП и тд соответствующая константа
* `system` : платёжная система (VISA, MasterCard и тд)
* `account` : маскированный номер карты

Важно: для этой функциональности не требуется `paymentToken` , так как осуществляется только выбор способа оплаты, без самой оплаты чего либо.

{% cut "Screenshots iOS" %}

![Список](_assets/select_ios_1.png "Список" =270x480)
![Редактирование](_assets/select_ios_2.png "Редактирование" =270x480)
![Загрузка](_assets/select_ios_3.png "Загрузка" =270x480)
![Успех](_assets/select_ios_4.png "Успех" =270x480)

{% endcut %}

{% cut "Screenshots Android" %}

![Список](_assets/select_android_1.png "Список" =270x480)
![Редактирование](_assets/select_android_2.png "Редактирование" =270x480)
![Загрузка](_assets/select_android_3.png "Загрузка" =270x480)
![Успех](_assets/select_android_4.png "Успех" =270x480)

{% endcut %}

{% cut "Code snippet iOS" %}
```
// создание переменной paymentKit см. в первом кейсе
var selectedOption: Payment.Option? // способ оплаты, который возвращается на экране выбора способа оплаты
let onItemSelected: (Payment.Option, Callbacks.SelectPaymentMethod.ItemSelectedActions) -> Void = { option, actions in
    // вызывается при каждом нажатии на способ оплаты, чтобы изменить состояние кнопки "Оплатить" или указать стоимость
    actions.setPaymentEnabled(true) // задаёт состояние кнопки "Оплатить" enabled/disabled
    actions.setPaymentData(Preselect.PurchaseData(sum: 999.99, total: 1000)) // можно указать сумму,  и как она изменится при выборе того или иного способа оплаты, например, корзина стоит 1000р, но при выборе карты МастерКард она начинает стоить 999.99
}
let onSelectionConfirmed: (Payment.Option?) -> Void = { option in selectedOption = option } //сохраняем выбранный способ оплаты
let callbacks = Callbacks.SelectPaymentMethod(onItemSelected: onItemSelected, onSelectionConfirmed: onSelectionConfirmed)
paymentKit.presentSelectMethodController(in: presentingController,
                                         animated: true,
                                         callbacks: callbacks,
                                         processCompletion: processCompletion,
                                         completion: nil) //открывается экран выбора способа оплаты без совершения самой оплаты
```
{% endcut %}


{% cut "Code snippet Android" %}
```
// создание переменной paymentKit см. в первом кейсе
startActivityForResult(
    paymentKit.createSelectMethodIntent<PreselectActivity>(),
    LAUNCH_PRESELECT_ACTIVITY
) //открывается экран выбора способа оплаты без совершения самой оплаты

override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    super.onActivityResult(requestCode, resultCode, data)
    if (requestCode == LAUNCH_PRESELECT_ACTIVITY) {
         if (resultCode == Activity.RESULT_OK && data != null) {
             // получаем выбранный способ оплаты
             val paymentOption = data.getParcelableExtra<PaymentOption>(ResultIntentKeys.DATA)
         }
    }
}
```
{% endcut %}

### Оплата предвыбранным способом

Кейс позволяет произвести оплату предвыбранным способом. Способ оплаты должен быть выбран заранее. Если выбранный способ не требует ввода дополнительных данных (оплата картой без CVV, Apple/GooglePay, СБП), то сразу начинается процесс оплаты.

{% cut "Screenshots iOS" %}

![Ввод CVV](_assets/preferred_ios_1.png "Ввод CVV" =270x480)
![Загрузка](_assets/preferred_ios_2.png "Загрузка" =270x480)
![Успех](_assets/preferred_ios_3.png "Успех" =270x480)

{% endcut %}

{% cut "Screenshots Android" %}

![Ввод CVV](_assets/preferred_android_1.png "Ввод CVV" =270x480)
![Загрузка](_assets/preferred_android_2.png "Загрузка" =270x480)
![Успех](_assets/preferred_android_3.png "Успех" =270x480)

{% endcut %}

{% cut "Code snippet iOS" %}
```
// создание переменной paymentKit см. в первом кейсе
paymentKit.presentPaymentController(for: token,
                                    preferredOption: selectedOption,
                                    in: presentingController,
                                    animated: true,
                                    processCompletion: processCompletion,
                                    completion: nil) // открывается диалог с единственным способом оплаты, который выбрал пользователь
```
{% endcut %}

{% cut "Code snippet Android" %}
```
// создание переменной paymentKit см. в первом кейсе
// storedPaymentOption - выбранный способ оплаты, сохранённый ранее
val paymentToken = PaymentToken("token") // id корзины, созданной в Трасте
startActivityForResult(
     paymentKit.createPaymentIntent<PaymentActivity>(paymentToken, storedPaymentOption),
     LAUNCH_PAYMENT_ACTIVITY
) // открывается диалог со способом оплаты, который выбрал пользователь, или сразу начинается оплата, если возможно
```
{% endcut %}

### Верификация карты

Кейс позволяет верифицировать банковскую карту, которая была привязана ранее. Для этого внутри СДК мы показываем форму ввода 3DS.

{% cut "Screenshots iOS" %}

![Загрузка](_assets/verify_ios_1.png "Загрузка" =270x480)
![3ds](_assets/verify_ios_2.png "3ds" =270x480)
![Поллинг](_assets/verify_ios_3.png "Поллинг" =270x480)
![Успех](_assets/verify_ios_4.png "Успех" =270x480)

{% endcut %}

{% cut "Screenshots Android" %}

![Загрузка](_assets/verify_android_1.png "Загрузка" =270x480)
![3ds](_assets/verify_android_2.png "3ds" =270x480)
![Поллинг](_assets/verify_android_3.png "Поллинг" =270x480)
![Успех](_assets/verify_android_4.png "Успех" =270x480)

{% endcut %}

{% cut "Code snippet iOS" %}
```
// создание переменной paymentKit см. в первом кейсе
let callbacks = Callbacks.BindCard { card in
    // данные верифицированной карты
}
paymentKit.presentVerifyCardController(for: "cardID",
                                       in: presentingController,
                                       animated: true,
                                       callbacks: callbacks,
                                       processCompletion: processCompletion,
                                       completion: nil)
```
{% endcut %}

{% cut "Code snippet Android" %}
```
// создание переменной paymentKit см. в первом кейсе
startActivityForResult(
    paymentKit.createVerifyCardIntent<BindCardActivity>(cardId),
    LAUNCH_VERIFY_ACTIVITY
) // показывается лоадинг, потом форма ввода 3DS

override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    super.onActivityResult(requestCode, resultCode, data)
    if (requestCode == LAUNCH_VERIFY_ACTIVITY) {
        if (resultCode == Activity.RESULT_OK) {
            // данные верифицированной карты
            var card = data.getParcelableExtra<BoundCard>(ResultIntentKeys.DATA)
        }
    }
}
```
{% endcut %}

### Список способов оплаты

Кейс позволяет получить доступные для пользователя способы оплаты. UI при этом не показывается, может быть полезен, если вы хотите самостоятельно отображать список способов оплаты.

{% cut "Code snippet iOS" %}
```
// создание переменной paymentKit см. в первом кейсе
paymentKit.paymentOptions { result in
    result.onValue { paymentOptions in
        // список способов оплаты, описывается структурой `Payment.Option`
    }.onError { error in
        // при выполнении запроса произошла ошибка
    }
}
```
{% endcut %}

{% cut "Code snippet Android" %}
```
// создание переменной paymentKit см. в первом кейсе
val result = paymentKit.paymentOptions()
when (result) {
    is Result.Success -> print(result.value) // список способов оплаты, описывается структурой `Payment.Option`
    is Result.Error -> print(result.error) // при выполнении запроса произошла ошибка
}
```
{% endcut %}

## Интеграция
### Запуск SampleApp

В семпле вы можете посмотреть, как выглядит форма оплаты, привязка карты, выбор способа оплаты и тп. Также там можно посмотреть, как интегрировать СДК в своё приложение. Актуальную версию семпла можно скачать тут. Данные для семпла можно сгенерировать с помощью скрипта generate_basket.sh  (подробности тут) или автоматически с помощью Test User Service (TUS).
Перед запуском семпла нужно получить TUS OAuth token. После этого запустить команду:
```
USER_SERVICE_OAUTH_TOKEN=<your token>
./scripts/prepare_properties.sh.
```

iOS
В iOS семпле для зависимостей используется CocoaPods , перед апдейтом подок нужно почистить кэш и удалить папку ./Pods:
```
pod cache clean  XPlatPaymentSDK --all
rm -rf ./Pods
pod update
```
Для работы с семплом:
1. Сгенерируйте тестовые данные
1. Запустите приложение
1. Убедитесь, что опция `Apply user config`  выключена, иначе автоматически будет получен пользователь из TUS.
1. Выберите `Authorization`  –> `Custom` , если вы не забыли выполнить скрипт перед запуском приложения, все поля кроме orderID  должны быть заполнены,
1. Нажмите "Pay" для показа формы оплаты, "Bind" для привязки карты без оплаты,

{% cut "Screenshots" %}

![Параметры](_assets/sample_ios_1.png "Параметры" =270x480)
![Сэмпл](_assets/sample_ios_2.png "Сэмпл" =270x480)

{% endcut %}

Android
В Android семпле используется `Gradle` , так что магия произойдёт автоматически, для работы с семплом:
1. Сгенерируйте тестовые данные
1. Запустите приложение
1. Убедитесь, что поля с тестовыми данными предзаполнены.
1. Нажмите "Pay" для показа формы оплаты, "Bind" для привязки карты без оплаты,

{% cut "Screenshot" %}

![Сэмпл](_assets/sample_android_1.png "Сэмпл" =270x480)

{% endcut %}

### Генерация тестовых данных

Если вам нужны тестовые данные для тестирования СДК или запуска семпла, их можно сгенерировать с помощью скрипта [generate_basket.sh](https://a.yandex-team.ru/arc_vcs/mobile/payment-sdk/scripts/generate_basket.sh). Для его работы вам понадобится тестовый пользователь (о том, как его завести, можно прочитать [тут](https://wiki.yandex-team.ru/users/salavat/Novye-testovye-polzovateli-dlja-PaymentSDK/)).

{% note warning %}

Скрипт создаёт тестовую корзину в Трасте, если передать туда реального пользователя, произойдёт ошибка, скрипт не умеет создавать продовскую корзину.

{% endnote %}

Перед запуском `generate_basket.sh` необходимо создать файлик `client_secret.txt` и положить его в папку со скриптом. Данные для `client_secret` можно взять [тут](https://wiki.yandex-team.ru/users/salavat/FAQ-po-bekendu-PaymentSDK/) (самый последний комментарий внизу страницы). В файл `client_secret.txt` нужно добавить только сам секрет (без каких-либо ключей).

Вызов скрипта и возможные опции:
`./generate_basket.sh -u=USER_NAME -p=PASSWORD`
`-g` – в результате работы скрипта будет сгенерирован файл `credentials.json` , из которого берет данные семпл, эта опция обязательна, если вы хотите запустить семпл.
`-3` – для созданной корзины будет запрошен 3DS
Остальные опции можно найти внутри самого скрипта.

{% note warning %}

Нужно брать актуальный скрипт из репозитория, а не хранить локальную копию у себя на машине, потому что мы обновляем этот скрипт время от времени.

{% endnote %}

### Возможные ошибки

При возникновении ошибок в процессе оплаты или работы самого СДК PaymentSDK  возвращает структуру `PaymentKitError` , которая содержит поля:
* `kind` : тип ошибки
* `trigger` : кто вызвал ошибку
* `code` : код ошибки, может быть `null` 
* `status` : строковый статус ошибки, может быть `null` 
* `message` : строка с подробным описанием ошибки

Возможные триггеры ошибок:
* `internal` : ошибка возникла внутри СДК
* `mobileBackend` : ошибка возникла на стороне нашего мобильного бэкенда
* `diehard` : ошибка возникла на стороне Траста
* `nspk` : ошибка возникла при обращении в НСПК

Возможные типы ошибок:
* `unknown` : мы не смогли понять, что именно произошло, возможно, бэкенд прислал статус, который мы не умеем обрабатывать, попробуйте получить больше информации из поля  `status` или `message` 
* `internalError` : ошибка работы самого СДК, о ней стоит сообщить в чат саппорта с описанием кейса, мы попробуем воспроизвести и исправить
* `authorization` : ошибка авторизации пользователя
* `network` : сетевая ошибка
* `bindingInvalidArgument` : ошибка карточных данных при привязке карты
* `tooManyCards` : привязано слишком много карт, сейчас Траст по техническим причинам не умеет привязывать более 5-ти карт, при привязки шестой можно получить такую ошибку
* `applePay` : ошибка при оплате `ApplePay`
* `googlePay` : ошибка при оплате `GooglePay`
* `fail3DS` : ошибка ввода 3DS
* `expiredCard` : срок действия каты истёк
* `invalidProcessingRequest` : ошибка в данных, переданных в платежную систему
* `limitExceeded` : достигнуты лимиты по операциям с картой (суточные\месячные ограничения, лимит на сумму транзакции и т.п.)
* `notEnoughFunds` : на карте недостаточно средств
* `paymentAuthorizationReject` : банк отклонил транзакцию, за подробностями надо обращаться в банк
* `paymentCancelled` : операция отменена на стороне партнера и далее
* `paymentGatewayTechnicalError` : техническая ошибка на стороне платежного шлюза
* `paymentTimeout` : платеж не авторизован, т.к. пользователь не ввел данные за отведенный срок
* `promocodeAlreadyUsed` : спец. статус для многоразовых промокодов – пользователь уже воспользовался данным промокодом
* `restrictedCard` : карта недействительна (украдена, утеряна и т.п.)
* `transactionNotPermitted` : операция недоступна для данной карты (установлены ограничения пользователем или банком). Например, запрет интернет-транзакций
* `userCancelled` : пользователь отказался от платежа
* `creditRejected` : кредит не одобрен (актуально для кредитов Тинькофф)
* `noEmail` : не передан валидный email
* `sbpBanksNotFound` : на устройстве не найдено ни одного банковского приложения (актуально при оплате предвыбранным методом СБП)
* `paymentMethodNotFound` : переданный предвыбранный метод отсутствует в разрешенных методах с бэкенда.


[Тут](https://wiki.yandex-team.ru/trust/payments/rc/) можно найти список ошибок оплаты, которые умеет возвращать Траст.

### Кастомизация

#### Android
Для того, чтобы изменить внешний вид формы, нужно:
* Создать свою тему на основе одной из существующих (`PaymentsdkYaTheme.Payments.Light`  или `PaymentsdkYaTheme.Payments.Dark` ), переопределить в ней необходимые атрибуты в [XML](https://a.yandex-team.ru/review/1542450/files/6#file-0-51646422:R12)
* Реализовать интерфейс `Theme`, передав туда свою [тему](https://a.yandex-team.ru/review/1542450/files/6#file-0-51646419:R476)
* Задать тему с помощью параметра `theme` при инициализации `PaymentKit`

Описание параметров в теме:
* `paymentsdk_background`  - цвет бэкграунда формы оплаты (color)
* `paymentsdk_payButtonBackground`  - бэкграунд кнопки оплаты (drawable)
* `paymentsdk_payButtonTextAppearance`  - text appearance для основного текста кнопки (style)
* `paymentsdk_payButtonTotalTextAppearance`  - text appearance для текст с суммой на кнопке (style)
* `paymentsdk_radioButton`  - бэкграунда радио баттона в виде selector'а с состоянием state_selected (drawable)
* `paymentsdk_gpayCardIconAccentColor`  - цвет текста "Pay" в лого Google Pay (color)
* `paymentsdk_navigationArrowColor`  - цвет стрелки навигации (color)
* `paymentsdk_brandIcon`  - иконка в верхней части формы оплаты. По умолчанию лого Яндекса. Для отключения использовать `@null` (drawable)
* `paymentsdk_brandButtonIcon`  - иконка на кнопке оплаты в виде selector'а с состоянием state_enabled. По умолчанию лого Яндекса. Для отключения использовать `@null`  (drawable)
* `paymentsdk_progressBarColor`  - цвет прогрессбара (color)
* `paymentsdk_closeIcon`  - иконка "крестика" (drawable)
С версии 2.1.0 атрибуты были изменены следующим образом:
* удалён атрибут `paymentsdk_brandingEnabled` в пользу `paymentsdk_brandIcon` и `paymentsdk_brandButtonIcon`. Для отключения брендинга, каждому из этих двух параметров нужно проставить `@null` 
* удалены атрибут `paymentsdk_payButtonTextColor` / `paymentsdk_payButtonTextSize` в пользу `paymentsdk_payButtonTextAppearance` и `paymentsdk_payButtonTotalTextAppearance` . Теперь можно задать не только цвет текста, но и полноценный Text Appearance. Стиль можно задать отдельно как для основного текста кнопки, так и для лейбла со стоимостью.

Для настройки текста сообщения о загрузке/успехе/не успехе операции и тп, можно использовать класс `TextProvider.Builder` с помощью функции `PaymentKit.updateTextProvider()` после инициализации `PaymentKit`. Он позволяет задать следующий текст:
* `setPaymentLoadingText`  – оплата в процессе
* `setPaymentSuccessText`  – оплата прошла успешно
* `setPaymentFailureText`  – оплата завершилась с ошибкой
* `setBindingLoadingText`  – привязка карты в процессе
* `setBindingSuccessText`  – привязка карты прошла успешно
* `setBindingErrorText`  – привязка карты завершилась с ошибкой
* `setUnbindingLoadingText`  – отвязка карты в процессе
* `setUnbindingSuccessText`  – отвязка карты прошла успешна
* `setUnbindingErrorText`  – отвязка карты завершилась с ошибкой
* `setPreselectLoadingText`  – выбор карты в процессе
* `setPreselectErrorText`  – выбор карты завершился с ошибкой
* `setCreditRejectText`  – в выдаче кредита отказано

#### iOS
В iOS для кастомизации UI используется класс `Theme.Builder` . При создании билдера нужно явно указать, на базе какой темы он будет построен (тёмной или светлой), из неё будут браться все не заданные значения. `Theme.Builder`  позволяет настраивать следующие элементы:
* `withPrimaryTintColor`  – цвет бэкграунда кнопки "Оплатить"
* `withActivityIndicatorColor`  – цвет лоадинга
* `withPaymentNormalTextColor`  – цвет текста на кнопке "Оплатить"
* `withBackgroundColor`  – цвет бэкграунда диалога
* `withRadioButtonImage`(on:off:)  – картинка для радио баттона в списке способов оплаты (выбранное и дефолтное состояние)
* `withActionButtonTitleFont`  – шрифт заголовка на кнопке "Оплатить"
* `withActionButtonSubtitleFont`  – шрифт подзаголовка на кнопке "Оплатить"
* `withActionButtonDetailsFont`  – шрифт деталей на кнопке "Оплатить"
* `withBrandLogo`  – картинка бренда в левом верхнем углу (по умолчанию "Яндекс")
* `shouldHideLogo`  – скрытие картинка бренда в левом верхнем углу

Code snippet
```// создание переменной paymentKit см. в первом кейсе
let builder = Theme.Builder(basedOn: .light)
    .withPrimaryTintColor(.red)
    .withBackgroundColor(.blue)
    .withActivityIndicatorColor(.green)
paymentKit.applyUITheme(.custom(builder))
```

Для настройки текста сообщения о загрузке/успехе/не успехе операции и тп, можно использовать класс `TextProvider.Builder`, он позволяет задать следующий текст:
* `setPaymentLoadingText`  – оплата в процессе
* `setPaymentSuccessText`  – оплата прошла успешно
* `setPaymentFailureText`  – оплата завершилась с ошибкой
* `setBindingLoadingText`  – привязка карты в процессе
* `setBindingSuccessText`  – привязка карты прошла успешно
* `setBindingFailureText`  – привязка карты завершилась с ошибкой
* `setUnbindingLoadingText`  – отвязка карты в процессе
* `setUnbindingSuccessText`  – отвязка карты прошла успешна
* `setUnbindingFailureText`  – отвязка карты завершилась с ошибкой
* `setVerificationLoadingText`  – верификация карты в процессе
* `setVerificationSuccessText`  – верификация карты прошла успешно
* `setVerificationFailureText`  – верификация карты завершилась с ошибклй
* `setCreditRejectedText`  – в выдаче кредита отказано

Code snippet
```
// создание переменной paymentKit см. в первом кейсе
let textProvider = TextProvider.Builder()
    .setPaymentLoadingText("in progress")
    .setPaymentSuccessText("success")
    .setPaymentFailureText("закончилась денежка, нищеброд")
paymentKit.applyTextProvider(textProvider)
```

### ApplePay

Для подключения `ApplePay` в `PaymentSDK`  необходимы следующие шаги:
* Создать в безопасном окружении ECC приватный ключ и сгенерировать CSR. Для этого нужно завести тикет на админов Траста, очередь [PAYSYSADMIN](https://st.yandex-team.ru/PAYSYSADMIN), тег **duty**, компонента **TRUST**, они создадут ключ и сертификат.
* Полученный CSR загрузить в Apple Developer Portal и получить [Apple Pay Merchant Identity Certificate](https://developer.apple.com/library/archive/ApplePay_Guide/Configuration.html#//apple_ref/doc/uid/TP40014764-CH2-SW1).
* Создать новый терминал, через который будут проходить ApplePay-платежи и прописать необходимые теги: `ApplePay` и apple_pay:<merchant_id>, где <merchant_id> - полученный от сервиса Merchant Idenitfier, с которым они создают apple_pay-криптограммы. Для этого нужно создать тикет в очереди [GMSP –> Запрос на добавление или изменение терминала](https://st.yandex-team.ru/createTicket?queue=GMSP&_form=58717). Note: терминалы нужны и для тестинга, и для прода.
* Обратиться в [чат поддержки](https://t.me/joinchat/ECJJh1Cv6Jj2g-r7XpCgwg) `PaymentSDK`, мы включим `ApplePay`  для тестирования на ваш сервис по `service_token` или же на одного пользователя по `uid` .
* Подключить `ApplePay`  в `PaymentSDK` ,
{% cut "code snippet" %}
```
var applePayConfig = AdditionalSettings.ApplePay(countryCode: "RU", appMerchantID: <YOUR_MERCHANT_ID>)
var builder = AdditionalSettings.Builder().setApplePay(applePayConfig)
let paymentKit = PaymentFactory
                     .setPaymentKitMode(.debug) /* для прода .release */
                     .initialize()
                     .makePaymentKit(
                         payer: payer,
                         merchant: merchant,
                         additionalSettings: builder.build()
                      )
```
{% endcut %}
* Необходимо проверить, что оплата работает и на тестинге, и в проде. Если на шаге 4. вы включали `ApplePay`  на одного пользователя, то нужно включить его на весь сервис.

Чуть подробнее про схемы подключения можно прочесть [тут](https://wiki.yandex-team.ru/trust/dev/applepaymanual/#applepay.v2).

### GooglePay

Для подключения `GooglePay` в `PaymentSDK`  по схеме `GooglePay.PAYMENT_GATEWAY` необходимы следующие шаги:
* Получить [merchant_id](https://support.google.com/paymentscenter/answer/7163092?hl=en) и `merchant_name` .
* Создать закрытый ключ, по нему приватный и публичный ключи. Для этого нужно завести тикет на админов Траста, очередь [PAYSYSADMIN](https://st.yandex-team.ru/PAYSYSADMIN), тег **duty**, компонента **TRUST**, и передать им `merchant_id`  и `merchant_name`. Публичный ключ отдаётся сервису.
* Создать новый терминал, через который будут проходить GooglePay-платежи и прописать необходимые теги: GooglePay и google_pay:<merchant_id>. Для этого нужно создать тикет в очереди [GMSP –> Запрос на добавление или изменение терминала](https://st.yandex-team.ru/createTicket?queue=GMSP&_form=58717). Note: терминалы нужны и для тестинга, и для прода.
* Обратиться в [чат поддержки](https://t.me/joinchat/ECJJh1Cv6Jj2g-r7XpCgwg) `PaymentSDK`, мы включим `GooglePay`  для тестирования на ваш сервис по `service_token`  или же на конкретного пользователя по `uid` .
* Подключить `GooglePay` в `PaymentSDK` ,
{% cut "code snippet" %}
```
var googlePayConfig = GooglePayGatewayData(<GATEWAY_ID>, <MERCHANT_ID>)
var additionalSettings = AdditionalSettings.Builder()
                .setGooglePayGatewayData(googlePayConfig)
                .build()
val paymentKit = factory.createPaymentKit(
            payer,
            merchant,
            additionalSettings
)
```
{% endcut %}
* Необходимо проверить, что оплата работает и на тестинге, и в проде. Если на шаге 4. вы включали `GooglePay` на одного пользователя, то нужно включить его на весь сервис.

Чуть подробнее про схемы подключения можно прочесть [тут](https://wiki.yandex-team.ru/trust/dev/google_pay_manual/#podkljuchenienovogoservisa).

### Family Pay

Payment SDK поддерживает оплату семейными картами.
Тикет - [PAYMENTSDK-605](https://st.yandex-team.ru/PAYMENTSDK-605)
Дизайн - [https://www.figma.com/file/O6IwI9l9cQHPQ9SDZUrjmn/1-MASTER-FILE-%F0%9F%94%92?node-id=5778%3A25634](https://www.figma.com/file/O6IwI9l9cQHPQ9SDZUrjmn/1-MASTER-FILE-%F0%9F%94%92?node-id=5778%3A25634)
Техническая информация - [https://wiki.yandex-team.ru/users/amosov-f/familypay](https://wiki.yandex-team.ru/users/amosov-f/familypay)

{% cut "Screenshots" %}

![iOS](_assets/family_ios_1.png "iOS" =270x480)
![Android](_assets/family_android_1.png "Android" =270x480)

{% endcut %}

Информация о семейной карточке и ее лимитах доступна клиентам через поля возвращаемых методов оплаты:
{% cut "Code snippet" %}
```
/**
 * Информация о владельце карты, доступной пользователю
 * Подробнее [по ссылке](https://wiki.yandex-team.ru/users/amosov-f/familypay/#variantformataotvetaruchkilpmvtraste)
 *
 * @property familyAdminUid uid владельца карты
 * @property familyId id семьи
 * @property expenses текущее использование пользователем карты (в копейках)
 * @property limit лимит карты, установленный родителем (в копейках)
 * @property currency валюта лимита, выбранная родителем (ISO 4217)
 * @property frame тип лимита. Пр. "month", "day"
 */
@Parcelize
class FamilyInfo(
    val familyAdminUid: String,
    val familyId: String,
    val expenses: Int,
    val limit: Int,
    val currency: String,
    val frame: String,

    /**
     * @property available Доступное для оплаты количество денег (с копейками после запятой)
     */
    val available: Double = (limit - expenses).toDouble() / 100
) : Parcelable

[ ... ]

data class Card(
    val id: CardId,
    val system: CardPaymentSystem,
    val account: String,
    val bankName: BankName,
    val familyInfo: FamilyInfo?
) : PaymentMethod()
```
{% endcut %}

Эту информацию, к пример, можно использовать, чтобы проверять и блокировать кнопку оплаты в [сценарии выбора способа оплаты (Preselect)](#выбор-способа-оплаты) в зависимости от доступного лимита на карте. Пример:
{% cut "Code snippet Android" $}
```
val changedOptionObserver = PaymentKitObserver<PaymentOption> { option: PaymentOption ->
    promise<Unit> { _, _ ->
        startActivity(
            paymentKit.createUpdatePreselectButtonStateIntent<PreselectActivity>(
                PreselectButtonState(false, 100.0, null)
            )
        )
          
        // some async work here, ex. create purchase, get total price ... 
        
        val available = option.familyInfo?.available ?: Double.MAX_VALUE
        val buttonState = PreselectButtonState(active = available >= 100.0, total = 100.0, subTotal = null)
        
        startActivity(
            paymentKit.createUpdatePreselectButtonStateIntent<PreselectActivity>(buttonState)
        )
    }
}
changePaymentOptionObserver.addObserver(changedOptionObserver)
```
{% endcut %}

{% cut "Code snippet iOS" %}
```
self.paymentKit?.presentSelectMethodController(
    in: self.rootController,
    animated: true,
    callbacks: Callbacks.SelectPaymentMethod(
        onItemSelected: self.paymentOptionSelectedCallback,
        onSelectionConfirmed: { _ in /* do something */ }
    ), processCompletion: self.processCompletion, completion: nil)

private func paymentOptionSelectedCallback(option: Payment.Option, actions: Callbacks.SelectPaymentMethod.ItemSelectedActions) {
    actions.setPaymentEnabled(false)
    actions.setPaymentData(nil)

    // some async work here, ex. create purchase, get total price ...
    
    let available = option.familyInfo?.available ?? Double.greatestFiniteMagnitude
    actions.setPaymentEnabled(available >= 100.0)
    actions.setPaymentData(Preselect.PurchaseData(sum: 100.0, total: nil))
}
```
{% endcut %}

### Система быстрых платежей

Для оплаты через СБП в Трасте должен быть [заведен терминал](https://wiki.yandex-team.ru/trust/sbp/#podkljuchenienovogoservisa) с его поддержкой.
После этого пункт с оплатой СБП появиться в списке доступных методов оплаты и при его выборе пользователь сможет выбрать банковское приложение (если на устройстве их несколько).

Подробнее [тут](sbp.md)

## Основные понятия
### Payer

Структура `Payer` содержит данные о пользователе, который осуществляет платёж:
* `oauth` : паспортный токен (optional)
* `uid` : паспортный uid (optional)
* `email` : адрес, на который будет выслан чек об оплате (optional)
Можно производить оплату и для пользователя, который незалогинен в приложение. Для этого оба поля `oauth` и `uid`  должны быть `null`.

{% note warning %}

Для залогиненного пользователя оба поля `oauth`  и `uid` должны быть не пустыми.

{% endnote %}


{% note warning %}

Важно: Для успешной оплаты так или иначе необходим `email` , так как на него будет выслан чек об оплате. В случае, если `email`  не был передан в классе `Payer`, необходимо настроить параметр `PersonalInfo` через `AdditionalSettings`. Таким образом пользователь сможет самостоятельно указать `email` в поле ввода.

{% endnote %}

### Merchant

Структура `Merchant` содержит данные о получателе платежа (сервис, в пользу которого будет сделан перевод). Например, при оплате заказа на Маркете, мерчантом выступает сервис Маркета и так далее:
* `serviceToken` : идентификатор сервиса в Трасте

{% note warning %}

`ServiceToken`  есть у всех яндексовых приложений, которые уже умеют платить через Траст.

{% endnote %}

Если вы не знаете свой `serviceToken` :
* Спросите у вашего бэкенда, если он умеет создавать корзину в Трасте, возможно, знает значение `serviceToken` .
* Возможно вы знаете номер вашего сервиса, по нему ребята из Траста смогут найти ваш `serviceToken`.

В Супераппе мерчантом выступает именно тот турбоапп, в котором в данный момент происходит платёж, например, Литрес, Игры, Лавка и тд. Но у внешних сервисов (Литрес, КупиВип) нет `serviceToken`  (они не общаются с Трастом напрямую, поэтому у них идентификатора внутри Траста), поэтому в это поле можно передавать пустую строку.
Сам СДК не использует `serviceToken`  для каких-то внутренних нужд, а просто логирует и передаёт его дальше в Траст.

### PaymentToken

Структура `PaymentToken`  содержит данные о корзине, которую пользователь будет оплачивать:
* token : идентификатор корзины (aka `purchase_token` ), которые возвращает Траст при вызове ручки `v2/payments`

Обычно корзину создаёт бэкенд, он же может вернуть на клиент этот идентификатор, и по нему внутри СДК мы осуществим оплату.

{% note warning %}

Для операций, не требующих оплату (привязка карты без оплаты, список способов оплаты), PaymentToken  не нужен.

{% endnote %}

### AdditionalSettings

Структура `AdditionalSettings`  позволяет задать дополнительные настройки для формы оплаты:
* `cardValidationConfig` : дополнительная валидация карточных данных (например, если вам нужно вводить только карты системы VISA).
* `paymentMethodsFilter` : позволяет явно отфильтровать способы оплаты, которые приходят с бэкенда (например, вы хотите показывать только карты, и не показывать СБП и тд).
* `appInfo` : информация о турбоаппе, нужно для аналитики.
* `hideFinalState` : скрывать экран успеха/ не успеха оплаты или привязки карты и сразу автоматически закрывать диалог.
* `enableCashPayments` : показывать оплату наличными в кейсе выбора способа оплаты.
* `showPersonalInfo` : показывать ли блок с вводом персональной информации (нужно для незалогинов). В блоке personal info пользователь так же может вручную ввести и настроить `email` (см. `PersonalInfoMode.SHOW_IF_HAS_NO_EMAIL` )
* [iOS only] `applePay` : настройка оплаты с помощью `ApplePay`.
* [Android only] `browserCards` : позволяет отобразить в форме оплаты карты, которые есть у пользователя в Браузере, но отсутствуют в Трасте.
* [Testing only] `forceCVV` : форсить показ CVV, даже если антифрод его не запросил, используется только в тестовых целях, данный функционал не поддерживается на проде.
* [Testing only] `passportToken` : паспортный токен пользователя для тестирования СДК ассесорами.

Все эти поля имеет дефолтное значение и не требуют явного создания.

### Траст, мобильный бэкенд и все–все–все

[Траст](https://wiki.yandex-team.ru/trust/) – это сервис, который осуществляет сам платёж, в нём также хранятся карточные данные и информация о том, какие способы оплапы доступны для определённого сервиса. Diehard мобильное АПИ Траста, мы ходим туда напрямую из СДК.

Мобильный бэкенд `PaymentSDK` служит прокси между Трастом и самим СДК. Мы получаем через него список доступных способов оплат (мобильный бэк получает этот список из Траста и отфильтровывает способы, которые мы не умеем обрабатывать внутри СДК), ходим в ручки, которые не поддержаны в Diehard  (например, /verify ). Все вызовы оплаты не проходят через мобильный бэкенд, они идут напрямую в Траст.
Также мобильный бэкенд стартует оплаты корзины.

{% note warning %}

Мобильный бэкенд не находится в контуре PCI DSS, это означает, что в нём **нельзя** хранить карточные данные или пересылать их через мобильный бэк.

{% endnote %}

[Тут](https://wiki.yandex-team.ru/personal-services/Oplata/SuperApp/#processprovedenijaplatezhapartneromvsuperappe) можно подробно рассмотреть схему взаимодействия всех компонентов при оплате на примере Супераппа. Кратко схема оплаты выглядит следующим образом:
1. При открытии диалога СДК идёт в мобильный бэк `/init_payment` 
1. Мобильный бэк стартует оплату корзины в Трасте
1. Мобильный бэк возвращает способы оплаты, СДК их отображает
1. Пользователь выбрал способ оплаты, нажал "Оплатить"
1. СДК досылает в Diehard  выбранный способ оплаты через `/supply_payment_data` 
1. СДК поллит статус платежа в Diehard  запрос `/check_payment`

Подробнее про поллинг статуса и показ 3DS можно прочесть [тут](https://wiki.yandex-team.ru/users/a-kononova/paymentsdk/howto-dev/#pokaz3dsipollingstatusa).


## Полезно знать
### Показ 3DS и поллинг статуса

На каждый платёж антифрод может запросить верификацию, в этом случае внутри СДК будет показана `WebView` с формой ввода 3DS. Для этого после начала платежа СДК поллит статус платежа. Если вы в логах видете много запросов на ручку `/check_payment `, так и задумано: мы поллим статус. В этом месте нам может прийти из Траста три состояния:
* `success` : платёж успешно завершен
* `error` : произошла ошибка, платёж не был осуществлён
* `wait_for_notification` : ждём нотификации об изменении статуса платежа, в этом случае нам так же может прийти `redirectUrl` , и тогда мы покажем форму для ввода 3DS.

{% note warning %}

Если вы видите в логах запросы `/check_payment`, но форма ввода 3DS так и не показалась (на форме оплаты крутится лоадинг):
* Нужно проверить, умеет ли ваш терминал запрашивать для вас 3DS (это лучше уточнить у команды Траста).
* Если вы создавали корзину с параметром `wait_for_cvv = 1`  (например, в тестинге использовали для создания корзины наш скрипт `generate_basket.sh`), но при оплате картой CVV не был запрошен, то `redirectUrl` тоже не приходит.

{% endnote %}

### Показ CVV

Мы **всегда** запрашиваем CVV при **привязке новой карты**, и запрашиваем его по согласно антифроду и нашему конфигу бэкенда при оплате существующей картой. Это означает, что поле ввода CVV будет показано, если нам придёт соответствующее поле в списке способов оплаты.

{% note tip %}

Сервис может повлиять на показ CVV, для этого нужно обратиться к антифроду Траста и обсудить правила показа для конкретного сервиса.

{% endnote %}

### Привязка карты с верификацией aka /v2.0/bindings

СДК поддерживает два АПИ для привязки карты:
* Старое АПИ `/bind_card` , которое не поддерживает верификацию при привязке карты. Не рекомендуется к использованию, работает только для ограниченного числа сервисов, будет удалено в последующих релизах.
* Новое АПИ `/bindings/v2.0/bindings`, которое поддерживает верификации. Больше о новом привязочном АПИ Траста можно прочесть [тут](https://wiki.yandex-team.ru/trust/newbindingapi/).

В обоих случаях ввод CVV обязательный, но форма 3DS будет показана только для `/v2.0/bindings`. Помимо верификации новое привязное АПИ шифрует карточные данные на клиенте перед отправкой. Дальше СДК (как и в случае с платежом) поллит статус с помощью ручки `/check_payment`.

Подробнее на [этой странице](../binding.md).

### Тестовая среда

Для тестирования СДК мы ходим на тестовый стенд мобильного бэкенда и тестовый `Diehard` . Для этого нужно указать мод `debug` :
iOS `PaymentFactory.setPaymentKitMode(.debug)`
Android `PaymentFactory.setEnvironment(..., PaymentSdkEnvironment.TESTING, ...)`
Никакие дополнительные доступы заказывать не надо: мобильный бэкенд и `Diehard`  открыты на `:allstaff`.

{% note warning %}

Доступ в тестовые среды возможен только из внутренней сети или под VPN.

{% endnote %}

Как посмотреть логи

Для дебага и для решения возникших проблем пригодятся логи запросов и ответов от сервера. В Android и в iOS их можно найти в Logcat/консоли. Для того, чтобы настроить логирование, необходимо создать сдк с параметром `ConsoleLoggingMode` .


### Скоупы токенов

Для авторизации в СДК пользовательские токены должны быть в скоупе `payments:all`. В случае, если не делать проверку скоупа, любое стороннее приложение сможет дергать API оплаты под аккаунтом пользователя (в том числе получать списки карт для оплаты), например, запросив у него доступ только на чтение email. Также, если приложение–хост где-то залогирует токен пользователя, то по этому токену можно будет получить доступ к данным пользователя как внутри этого приложения, так и внутри СДК.


Изначально при подключении Супераппа нужный скоуп был добавлен к client_id Супераппа, все старые пользователи были размечены вручную. СИБ был закономерно против использования такого подхода. С версии 1.5.0 в СДК добавился функционал [PAYMENTSDK-252](https://st.yandex-team.ru/PAYMENTSDK-252), который позволяет обменивать паспортный токен на токен с нужным скоупом. Если вы используете СДК версии ниже, чем 1.5.0, вам нужно позаботится о том, чтобы токены для всех ваших пользователей (новых и старых) бьыли в нужном скоупе.

Для включения функционала обмена скоупа в Android  вам нужно включить настройку `exchangeOauthToken` в `AdditionalSettings` , обмен скоупа произойдёт автоматически внутри СДК.

В iOS  вам нужно самостоятельно реализовать протокол `PaymentKitAuthExchanger` , обменять токен в Паспорте, а затем передать в СДК новы токен.

{% cut "Пример реализации PaymentKitAuthExchanger" %}
```swift
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

{% note warning %}

Если после успешного обмена токенов diehard всё равно возвращает ошибки со статусом 401 (`invalid scope`), нужно убедиться, что ваш сервис в АМ имеет признак `client_is_yandex = 1` ([пример обращения](https://st.yandex-team.ru/OAUTHREG-523#62948d91ac6573518d253e1f))

{% endnote %}

Немного подробнее об этом [тут](../oauth.md)

### Безопасность

СДК проходит сертификацию **PCI DSS**, каждый релиз ревьюится командой СИБа, вся наша команда прошла дополнительное обучение.

## Куда писать
Если вы хотите интегрировать PaymentSDK в ваше приложение, напишите, пожалуйста, [Ольге Демаковой](https://staff.yandex-team.ru/olyd). Даже если у вас нет вопросов, нам обязательно нужно знать, какие сервисы с нами интегрируются, чтобы включить в свои планы возможные доработки и саппорт.

Вопросы по интеграции, функционалу и разработке направляйте в [чат саппорта]((https://t.me/joinchat/ECJJh1Cv6Jj2g-r7XpCgwg)) или дежурному.

Вопросы по срокам выполнения задач, планам команды, возможности взять в разработку ту или иную задачу, направляйте Ольге Демаковой.

Нашли ошибку в СДК - заводите тикет через [единую форму](https://forms.yandex-team.ru/surveys/74237/), появились идеи, как улучшить СДК - заводите тикет в очередь [PAYMENTSDK](https://st.yandex-team.ru/PAYMENTSDK).

## FAQ
`PaymentSDK разрабатывается командой Траста?`

Нет, PaymentSDK разрабатывается отдельной командой, это означает, что мы можем подсказать, к кому лучше обратиться по какому вопросу, но мы не можем отвечать за то, что происходит в Трасте. Аналогично ребята из Траста очень примерно понимают что и как у нас работает, так что не смогут ответить на ваши вопросы по СДК.

`Почему мы передали пустую строку в serviceToken , и всё работает?`

Скорей всего потому, что перед этим ваш бэкенд создал корзину в Трасте, указав `serviceToken`. Дальше вы передали нам идентификатор корзины в структуре `PaymentToken` , мы передали его в Траст, а по нему Траст уже понял, какой это сервис. Лучше всегда явно передавать свой `serviceToken`, даже если оплата работает и без него, потому что мы логируем это значение для получения статистики.

`Можно ли добавить свою логику по кнопку "Оплатить"/"Добавить карту"?`

В общем случае нет, оплата, добавление карты и прочий функционал реализован на стороне СДК. Если вам необходимо получить информацию о том, что пользователь нажал на кнопку, мы можем обсудить добавление некоторого коллбэка для вас.

`Можно ли кастомизировать внешний вид формы или отключить брендирование?`

Да, можно, в версии СДК 1.5.0 появилась возможность настраивать внешний вод кнопки "Оплатить" и выключать брендирование формы. Дальше мы планируем расширять этот функционал для более гибкой настройки формы оплаты.

`Можно ли отменить заказ?`

Нет, в СДК нет такого функционала, если закрыть диалог до того, как оплата будет завершена, то возможны два сценария:
* Если оплата не требует верификации (подробнее об этом можно прочесть [тут](https://wiki.yandex-team.ru/users/a-kononova/paymentsdk/howto-dev/#pokaz3dsipollingstatusa), то она будет успешно (или неуспешно) завершена.
* Если оплата требует верификации, то при закрытии диалога это состояние не будет обработано, и платёж завершиться с ошибкой.

```
Я получаю ошибку:
{"status":"incorrect format","code":1001,"req_id":"1626381523733955-7103786158336680608","message":"authorization failed"} 
что делать?
```
Скорей всего, переданный токен не содержится в скоупе :paymentall, см. раздел ["Скоупы"](#скоупы-токенов).
