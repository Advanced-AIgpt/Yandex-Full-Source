//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment](../index.md)/[SelectMethodMediator](index.md)

# SelectMethodMediator

[datasource]\
open class [SelectMethodMediator](index.md) : [SelectPaymentAdapter.PaymentMethodClickListener](../../../../ui/ui/com.yandex.payment.sdk.ui.view.payment/-select-payment-adapter/-payment-method-click-listener/index.md)

Медиатор для облегчения процесса выбора платёжного метода. Используется с [SelectPaymentAdapter](../../../../ui/ui/com.yandex.payment.sdk.ui.view.payment/-select-payment-adapter/index.md).

## Constructors

| | |
|---|---|
| [SelectMethodMediator](-select-method-mediator.md) | [datasource]<br>fun [SelectMethodMediator](-select-method-mediator.md)() |

## Functions

| Name | Summary |
|---|---|
| [getDataForSelect](get-data-for-select.md) | [datasource]<br>open fun [getDataForSelect](get-data-for-select.md)(): List<[SelectPaymentAdapter.Data](../../../../ui/ui/com.yandex.payment.sdk.ui.view.payment/-select-payment-adapter/-data/index.md)><br>Получить данные для адаптера. |
| [getSelectedIndex](get-selected-index.md) | [datasource]<br>open fun [getSelectedIndex](get-selected-index.md)(): Int?<br>Индекс текущего выбранного метода или null, если ничего не выбрано. |
| [onChangeCvn](on-change-cvn.md) | [datasource]<br>override fun [onChangeCvn](on-change-cvn.md)(position: Int, isValid: Boolean, cvnInput: [CvnInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/index.md)) |
| [onSelectPaymentMethod](on-select-payment-method.md) | [datasource]<br>open override fun [onSelectPaymentMethod](on-select-payment-method.md)(position: Int) |
| [provideCvn](provide-cvn.md) | [datasource]<br>fun [provideCvn](provide-cvn.md)(paymentApi: [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md))<br>Вызвать передачу CVC/CVV кода для текущей карты. |
| [reset](reset.md) | [datasource]<br>fun [reset](reset.md)()<br>Сбросить состояние. |
| [setCardCvnChecker](set-card-cvn-checker.md) | [datasource]<br>fun [setCardCvnChecker](set-card-cvn-checker.md)(api: [CardCvnChecker](../../com.yandex.payment.sdk.datasource.payment.interfaces/-card-cvn-checker/index.md)?)<br>Установить реализацию класса для проверки необходимости ввода CVC/CVV кода для конкретной карты. |
| [setMethods](set-methods.md) | [datasource]<br>open fun [setMethods](set-methods.md)(methods: List<[PaymentMethod](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md)>)<br>Задать список методов для отображения и работы. |
| [setMethodSelectionCallback](set-method-selection-callback.md) | [datasource]<br>fun [setMethodSelectionCallback](set-method-selection-callback.md)(callback: ([PaymentMethod](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md)) -> Unit?)<br>Задать коллбек на выбор метода. |
| [setPaymentButton](set-payment-button.md) | [datasource]<br>fun [setPaymentButton](set-payment-button.md)(button: [PaymentButton](../../com.yandex.payment.sdk.datasource.payment.interfaces/-payment-button/index.md)?)<br>Задать реализацию кнопки оплаты. |

## Properties

| Name | Summary |
|---|---|
| [currentMethod](current-method.md) | [datasource]<br>var [currentMethod](current-method.md): [PaymentMethod](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md)? = null<br>Текущий выбранный метод. |
