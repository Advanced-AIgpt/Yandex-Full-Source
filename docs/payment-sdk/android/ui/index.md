# Prebuilt UI компоненты

Содержит различные кастомизируемые Prebuilt UI компоненты: есть открытые, которые можно вставлять напрямую в xml разметку, и есть закрытые, которые можно создать только через специальную фабрику (компоненты для работы с данными банковских карт).

## Подключение
Для подключения добавьте зависимость от модуля `ui`
```
implementation 'com.yandex.paymentsdk:ui:$versions.paymentsdk'
```

## Открытые компоненты
Это кнопка оплаты [PaymentButtonView](../../ui/ui/com.yandex.payment.sdk.ui.view.payment/-payment-button-view/index.md), вебвью для 3ds [PaymentSdkWebView](../../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-web-view/index.md), вебвью для кредитов Тинькофф [PaymentSdkTinkoffWebView](../../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-tinkoff-web-view/index.md), а также расширяемый `RecyclerView` адаптер [SelectPaymentAdapter](../../ui/ui/com.yandex.payment.sdk.ui.view.payment/-select-payment-adapter/index.md) для работы со списком методов оплат.

## Компоненты для работы с данными банковских карт
Это [CardInputView](../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md) для работы со вводом всех данных карты и [CvnInputView](../../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/index.md) для ввода только CVV/CVC кода при оплате существующей картой. Создавать их можно с помощью фабрики, полученной из [createPrebuiltUiFactory](../../ui/ui/com.yandex.payment.sdk.ui/create-prebuilt-ui-factory.md).

## Кастомизация
На данный момент кастомизация происходит посредством указания атрибутов в темах. Для закрытых компонентов тема указывается при создании фабрики [createPrebuiltUiFactory](../../ui/ui/com.yandex.payment.sdk.ui/create-prebuilt-ui-factory.md).

Параметр | Описание | Значение
:--- | :--- | :---:
android:textColorPrimary | Цвет основного текста | color
android:textColorSecondary | Цвет дополнительного текста | color
paymentsdk_textCursorColor | Цвет текстового курсора | color
paymentsdk_errorInputColor | Цвет ошибочного текста | color
paymentsdk_textFieldTintColor | Цвет текста в инпуте | color
paymentsdk_navigationArrowColor | Цвет навигационной стрелки | color
paymentsdk_radioButton | Картинка для радиобаттонов | drawable
paymentsdk_brandButtonIcon | Картинка для брендовой кнопки оплаты | drawable
paymentsdk_payButtonBackground | Фон кнопки оплаты | drawable
paymentsdk_payButtonTextAppearance | Параметры текста на кнопке оплаты | reference
paymentsdk_payButtonTotalTextAppearance | Параметры текста для цены на кнопке оплаты | reference
paymentsdk_payButtonSubtotalTextAppearance | Параметры текста для промежуточной цены на кнопке оплаты | reference
paymentsdk_paymentCellBackground | Отдельный цвет для ячеек в списке методов | color
paymentsdk_paymentCellTextColor | Отдельный цвет для названия методов в списке | color
paymentsdk_prebuilt_cardNumberHintColor | Отдельный цвет для подсказки в форме ввода данных карт | color
paymentsdk_prebuilt_cardNumberPlaceholderColor | Отдельный цвет для плейсхолдера в форме ввода данных карт | color
paymentsdk_prebuilt_cardNumberTextColor | Отдельный цвет текста в форме ввода данных карт | color