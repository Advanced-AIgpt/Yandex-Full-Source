//[ui](../../index.md)/[com.yandex.payment.sdk.ui](index.md)

# Package com.yandex.payment.sdk.ui

## Types

| Name | Summary |
|---|---|
| [CardInput](-card-input/index.md) | [ui]<br>interface [CardInput](-card-input/index.md)<br>Интерфейс для работы с логикой ввода данных банковских карт. |
| [CardInputMode](-card-input-mode/index.md) | [ui]<br>enum [CardInputMode](-card-input-mode/index.md) : Enum<[CardInputMode](-card-input-mode/index.md)> <br>Возможные виды вью для ввода данных банковских карт. |
| [CardInputView](-card-input-view/index.md) | [ui]<br>abstract class [CardInputView](-card-input-view/index.md)@JvmOverloads()constructor(**context**: Context, **attrs**: AttributeSet?, **defStyleAttr**: Int) : FrameLayout, [CardInput](-card-input/index.md)<br>Абстрактный класс вью ввода данных банковских карт, реализация, возвращаемая в [PrebuiltUiFactory.createCardInputView](-prebuilt-ui-factory/create-card-input-view.md) будет наследоваться от него. |
| [CvnInput](-cvn-input/index.md) | [ui]<br>interface [CvnInput](-cvn-input/index.md)<br>Интерфейс для работы с логикой ввода CVC/CVV. |
| [CvnInputView](-cvn-input-view/index.md) | [ui]<br>abstract class [CvnInputView](-cvn-input-view/index.md)@JvmOverloads()constructor(**context**: Context, **attrs**: AttributeSet?, **defStyleAttr**: Int) : FrameLayout, [CvnInput](-cvn-input/index.md)<br>Абстрактный класс вью ввода CVC/CVV, реализация, возвращаемая в [PrebuiltUiFactory.createCvnInputView](-prebuilt-ui-factory/create-cvn-input-view.md) будет наследоваться от него. |
| [PrebuiltUiFactory](-prebuilt-ui-factory/index.md) | [ui]<br>interface [PrebuiltUiFactory](-prebuilt-ui-factory/index.md)<br>Интерфейс фабрики для создания секьюрных Prebuilt UI компонентов, таких как ввод данных банковских карт и отдельно CVV. |

## Functions

| Name | Summary |
|---|---|
| [createPrebuiltUiFactory](create-prebuilt-ui-factory.md) | [ui]<br>fun [createPrebuiltUiFactory](create-prebuilt-ui-factory.md)(@StyleRes()themeResId: Int): [PrebuiltUiFactory](-prebuilt-ui-factory/index.md)<br>Создать фабрику для Prebuilt UI компонентов. |
| [formatSum](format-sum.md) | [ui]<br>fun [formatSum](format-sum.md)(total: Double): String<br>fun [formatSum](format-sum.md)(context: Context, settings: [PaymentSettings](../../../core/core/com.yandex.payment.sdk.core.data/-payment-settings/index.md)): String<br>fun [formatSum](format-sum.md)(context: Context, total: Double, currencyCode: String): String<br>fun [formatSum](format-sum.md)(context: Context, total: String, currencyCode: String): String |
| [getCurrencySymbol](get-currency-symbol.md) | [ui]<br>fun [getCurrencySymbol](get-currency-symbol.md)(currencyCode: String): String |
| [getFormatterTotal](get-formatter-total.md) | [ui]<br>fun [PaymentSettings](../../../core/core/com.yandex.payment.sdk.core.data/-payment-settings/index.md).[getFormatterTotal](get-formatter-total.md)(): String |
