//[ui](../../../index.md)/[com.yandex.payment.sdk.ui.view.payment](../index.md)/[SelectPaymentAdapter](index.md)

# SelectPaymentAdapter

[ui]\
open class [SelectPaymentAdapter](index.md)(**listener**: [SelectPaymentAdapter.PaymentMethodClickListener](-payment-method-click-listener/index.md), **prebuiltUiFactory**: [PrebuiltUiFactory](../../com.yandex.payment.sdk.ui/-prebuilt-ui-factory/index.md), **isLightTheme**: Boolean, **mode**: [SelectPaymentAdapter.AdapterMode](-adapter-mode/index.md)) : RecyclerView.Adapter<[SelectPaymentAdapter.BaseViewHolder](-base-view-holder/index.md)> 

Адаптер для работы со списком платёжных методов. Содержит логику по работе с вьюшками ввода CVC/CVV кода. Может быть расширен для добавления новых ViewHolder'ов и типов данных.

## Parameters

ui

| | |
|---|---|
| listener | основной листенер взаимодействий с вьюшками. |
| prebuiltUiFactory | фабрика создания Prebuilt UI вьюшек, нужна для создания вью ввода CVC/CVV. |
| isLightTheme | использовать ли светлую тему. |
| mode | режим отображения иконок в адаптере. |

## Constructors

| | |
|---|---|
| [SelectPaymentAdapter](-select-payment-adapter.md) | [ui]<br>fun [SelectPaymentAdapter](-select-payment-adapter.md)(listener: [SelectPaymentAdapter.PaymentMethodClickListener](-payment-method-click-listener/index.md), prebuiltUiFactory: [PrebuiltUiFactory](../../com.yandex.payment.sdk.ui/-prebuilt-ui-factory/index.md), isLightTheme: Boolean, mode: [SelectPaymentAdapter.AdapterMode](-adapter-mode/index.md))<br>основной листенер взаимодействий с вьюшками. |

## Types

| Name | Summary |
|---|---|
| [AdapterMode](-adapter-mode/index.md) | [ui]<br>enum [AdapterMode](-adapter-mode/index.md) : Enum<[SelectPaymentAdapter.AdapterMode](-adapter-mode/index.md)> <br>Перечисление режимов отображения иконок в адаптере. |
| [BaseViewHolder](-base-view-holder/index.md) | [ui]<br>abstract class [BaseViewHolder](-base-view-holder/index.md)(**view**: View) : RecyclerView.ViewHolder<br>Базовый класс ViewHolder. |
| [Companion](-companion/index.md) | [ui]<br>object [Companion](-companion/index.md) |
| [Data](-data/index.md) | [ui]<br>interface [Data](-data/index.md)<br>Интерфейс для передаваемых данных. |
| [PaymentMethodClickListener](-payment-method-click-listener/index.md) | [ui]<br>interface [PaymentMethodClickListener](-payment-method-click-listener/index.md) |
| [PaymentSdkData](-payment-sdk-data/index.md) | [ui]<br>data class [PaymentSdkData](-payment-sdk-data/index.md)(**method**: [PaymentMethod](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md), **needCvn**: Boolean, **isUnbind**: Boolean) : [SelectPaymentAdapter.Data](-data/index.md)<br>Основная реализация данных для отображения - отображает платёжный метод. |

## Functions

| Name | Summary |
|---|---|
| [createCustomViewHolder](create-custom-view-holder.md) | [ui]<br>open fun [createCustomViewHolder](create-custom-view-holder.md)(parent: ViewGroup, viewType: Int): [SelectPaymentAdapter.BaseViewHolder](-base-view-holder/index.md)<br>Перегрузите, если хотите отображать свои ViewHolder. |
| [getItemCount](get-item-count.md) | [ui]<br>override fun [getItemCount](get-item-count.md)(): Int |
| [getItemId](get-item-id.md) | [ui]<br>open override fun [getItemId](get-item-id.md)(position: Int): Long |
| [getItemViewType](get-item-view-type.md) | [ui]<br>open override fun [getItemViewType](get-item-view-type.md)(position: Int): Int |
| [onBindViewHolder](on-bind-view-holder.md) | [ui]<br>override fun [onBindViewHolder](on-bind-view-holder.md)(holder: [SelectPaymentAdapter.BaseViewHolder](-base-view-holder/index.md), position: Int) |
| [onCreateViewHolder](on-create-view-holder.md) | [ui]<br>override fun [onCreateViewHolder](on-create-view-holder.md)(parent: ViewGroup, viewType: Int): [SelectPaymentAdapter.BaseViewHolder](-base-view-holder/index.md) |
| [setData](set-data.md) | [ui]<br>fun [setData](set-data.md)(methods: List<[SelectPaymentAdapter.Data](-data/index.md)>, selected: Int? = null, focusFirstCvv: Boolean = false)<br>Проставить данные для отображения. |

## Properties

| Name | Summary |
|---|---|
| [isCvnValid](is-cvn-valid.md) | [ui]<br>var [isCvnValid](is-cvn-valid.md): Boolean = false<br>Валиден ли текущий отображаемый CVC/CVV. |
| [selectedMethod](selected-method.md) | [ui]<br>var [selectedMethod](selected-method.md): [SelectPaymentAdapter.Data](-data/index.md)? = null<br>Текущий выбранный метод. |
