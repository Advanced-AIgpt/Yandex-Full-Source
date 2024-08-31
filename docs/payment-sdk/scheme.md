# Схема работы с данными карт

Основные идеи:
1. Наружу никакие чувствительные данные не проходят - от карточек только трастовый card id и маскированный PAN
1. Снаружи нельзя вызывать никакие методы, которые бы потребовали передачи Хостом карточных данных. То есть можно попросить GooglePay, бонусами Спасибо, уже привязанной новой картой и т.д., но нельзя напрямую передать данные для оплаты новой картой или даже CVV для оплаты существующей картой, если он запрошен

Создать кастомизированный компонент для ввода данных новой карты, или для ввода CVV кода в случае его запроса Трастом можно с помощью фабрики `PrebuiltUiFactory` в [Android](android/reference/ui/ui/com.yandex.payment.sdk.ui/-prebuilt-ui-factory/index.md) и [iOS](ios/reference/ui/PrebuiltUiFactory.md).
Снаружи компонент выглядит как простой `FrameLayout`/`UIView`, с некоторыми дополнительными методами.

`CardInput` ([Android](android/reference/ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md), [iOS](ios/reference/ui/CardInput.md)) используется для ввода всех данных карты при привязке карты или при оплате новой картой. `CvnInput`([Android](android/reference/ui/ui/com.yandex.payment.sdk.ui/-cvn-input/index.md), [iOS](ios/reference/ui/CvnInput.md)) может использоваться если хост хочет оплатить существующей картой и ему надо дать пользователю ввести CVV/CVC код. Его внешний вид совпадает с текущим видом в PaymentSDK, который используется в списке методов оплат.
У `CardInput` и `CvnInput` немного разный интерфейс ([onReadyListener](android/reference/ui/ui/com.yandex.payment.sdk.ui/-cvn-input/set-on-ready-listener.md)/[setOnReadyCallback](ios/reference/ui/CvnInput.md#setonreadycallback​) и [onStateChangeListener](android/reference/ui/ui/com.yandex.payment.sdk.ui/-card-input/set-on-state-change-listener.md)/[setOnStateChangedCallback](ios/reference/ui/CardInput.md#setonstatechangecallback​)) так как форма ввода карты разбита на два состояния, а ввод CVV содержит только один текстовый инпут.

Реализации данных компонентов находятся внутри PaymentSDK за модификатором internal и недоступны для прямого использования.
Хост-приложение:
1. размещает в своём лэйауте этот компонент,
1. соединяет его с построенным `PaymentApi` через `setPaymentApi`,
1. устанавливает листенер/коллбек на заполнение ([setOnReadyListener]((android/reference/ui/ui/com.yandex.payment.sdk.ui/-cvn-input/set-on-ready-listener.md)) / [setOnReadyCallback](ios/reference/ui/CvnInput.md#setonreadycallback​) или [setOnStateChangeListener](android/reference/ui/ui/com.yandex.payment.sdk.ui/-card-input/set-on-state-change-listener.md) / [setOnStateChangedCallback](ios/reference/ui/CardInput.md#setonstatechangecallback​)) - он сработает, когда введенные данные пройдут валидацию,
1. запускает процесс передачи данных в sdk (`provideCvn` / `provideCardData`)

{% cut "Схема для CVV" %}

![Схема для CVV](_assets/cvv_scheme.png "Схема для CVV" =700x640)

{% endcut %}

{% cut "Схема для карт" %}

![Схема для карт](_assets/card_scheme.png "Схема для карт" =760x640)

{% endcut %}

Последовательность в целом можно немного менять в угоду интерфейсу - например показывать ввод карточных данных заранее, делать ещё какие-то действия по мере заполнения (блокировать или разблокировать кнопку оплаты) - главное, что для проведения оплаты или привязки нужно вызвать метод `provide*` на готовой вью с проставленным внутрь `PaymentApi`.