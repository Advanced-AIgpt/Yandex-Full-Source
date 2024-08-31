# Quick start или как быстро чем-нибудь заплатить

## Что нужно иметь
* у вас должен быть `service token` вашего сервиса в Trust
* в приложении должен быть залогин через тестовый Паспорт
* у тестового пользователя должна быть привязана хоть одна карта
* создайте корзину и получите `purchase_token`

Теперь попробуем в качестве примера заплатить пользовательской банковской картой

## Основные объекты
* Основная точка входа в апи - [PaymentApi](../core/core/com.yandex.payment.sdk.core/-payment-api/index.md)
* Объект для управления конкретным платежом, получается после старта платежа - [Payment](../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/index.md)
* Медиатор для оплаты - [SimplePaymentMediator](../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-simple-payment-mediator/index.md)
* Интерфейс вью для ввода CVV/CVC - [CvnInput](../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/index.md)

## Шаг 1 - Реализуйте интерфейсы
Создайте медиатор [SimplePaymentMediator](../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-simple-payment-mediator/index.md) и реализуйте для него:
1. [PaymentProcessing](../datasource/datasource/com.yandex.payment.sdk.datasource.payment.interfaces/-payment-processing/index.md) - нужно в [pay](../datasource/datasource/com.yandex.payment.sdk.datasource.payment.interfaces/-payment-processing/pay.md) вызвать соответствующий метод [Payment](../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/index.md), передав туда первую доступную банковскую карту пользователя. Получить её можно вызвав [Payment#methods](../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/methods.md) и забрав оттуда любой метод типа [PaymentMethod.Card](../core/core/com.yandex.payment.sdk.core.data/-payment-method/-card/index.md)
1. [PaymentScreen](../datasource/datasource/com.yandex.payment.sdk.datasource.payment.interfaces/-payment-screen/index.md) - реализуйте в вашем сэмпле отображение состояний экрана оплаты.
1. [WebView3ds](../datasource/datasource/com.yandex.payment.sdk.datasource.bind.interfaces/-web-view3ds/index.md) - реализуйте показ и скрытие страницы 3ds - можете, например, отобразить её в нашем [PaymentSdkWebView](../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-web-view/index.md). Можно добавлять её в лэйаут экрана платежей или показать в отдельном фрагменте - как вам удобнее.

## Шаг 2 - Инициализация Api
Проинициализируйте PaymentApi как описано [здесь](core/initialization.md) в тестинге `PaymentSdkEnvironment.TESTING`, при сборке укажите чтоб был включен обмен скоупов вызвав в билдере метод [exchangeOauthToken](../core/core/com.yandex.payment.sdk.core/-payment-api/-builder/exchange-oauth-token.md) с `true`.
Это нужно чтобы мы обменяли ваш токен Oauth на токен со скоупом платежей.

Также не забудьте указать коллбеки [paymentCallbacks](../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-simple-payment-mediator/payment-callbacks.md) из медиатора.

## Шаг 3 - Старт платежа
Выполните старт платежа как описано в первом раздете вот [тут](core/payment.md). Вам понадобится полученный объект [Payment](../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/index.md).

## Шаг 4 - Обработайте ввод CVV
Создайте [CvnInput](../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/index.md) через [createCvnInputView](../ui/ui/com.yandex.payment.sdk.ui/-prebuilt-ui-factory/create-cvn-input-view.md) (см также страницу [модуля ui](ui/index.md)) и проставьте в него [setPaymentApi](../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/set-payment-api.md).

Отображать его не всегда обязательно - нужен ли ввод CVV/CVC для конкретной карты можно узнать через метод [Payment#shouldShowCvv](../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/should-show-cvv.md). Если окажется не нужен - можете его скрыть. Если нужен - отобразите в своем лэйауте и блокируйте кнопку оплаты, пока не придёт `true` в [onReadyListener](../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/set-on-ready-listener.md).

## Шаг 5 - Соединяем всё вместе
Проставьте в медиатор всё необходимое через [connectUi](../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-simple-payment-mediator/connect-ui.md).

В итоге ваш сэмпл должен выполнить инициализацию и старт платежа, показать ввод CVV/CVC если это необходимо и ожидать нажатия пользователем на кнопку оплаты, при котором надо вызвать [onPayClick](../datasource/datasource/com.yandex.payment.sdk.datasource.payment/-simple-payment-mediator/on-pay-click.md) у медиатора.

## Что дальше
Попробуйте добавить отображение списка методов оплаты, посмотрите как привязывать банковские карты, как оплачивать через другие поддерживаемые способы оплаты.

Ознакомьтесь с модулем с [медиаторами](datasource/index.md), модулем с [UI компонентами](ui/index.md) и основным [модулем api](core/index.md).
