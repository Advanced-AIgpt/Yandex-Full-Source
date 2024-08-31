//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[PaymentSettings](index.md)

# PaymentSettings

[core]\
class [PaymentSettings](index.md)(**total**: String, **currency**: String, **licenseURL**: Uri?, **acquirer**: [Acquirer](../-acquirer/index.md)?, **environment**: String, **merchantInfo**: [MerchantInfo](../-merchant-info/index.md)?, **payMethodMarkup**: [PaymethodMarkup](../-paymethod-markup/index.md)?, **creditFormUrl**: String?) : Parcelable

Подробная информация о платеже.

## Constructors

| | |
|---|---|
| [PaymentSettings](-payment-settings.md) | [core]<br>fun [PaymentSettings](-payment-settings.md)(total: String, currency: String, licenseURL: Uri?, acquirer: [Acquirer](../-acquirer/index.md)?, environment: String, merchantInfo: [MerchantInfo](../-merchant-info/index.md)?, payMethodMarkup: [PaymethodMarkup](../-paymethod-markup/index.md)?, creditFormUrl: String?) |

## Properties

| Name | Summary |
|---|---|
| [acquirer](acquirer.md) | [core]<br>val [acquirer](acquirer.md): [Acquirer](../-acquirer/index.md)?<br>обработчик платежа, для Яндекс.Оплат. |
| [creditFormUrl](credit-form-url.md) | [core]<br>val [creditFormUrl](credit-form-url.md): String?<br>ссылка на форму кредита, для кредитов Тинькофф. |
| [currency](currency.md) | [core]<br>val [currency](currency.md): String<br>валюта. |
| [environment](environment.md) | [core]<br>val [environment](environment.md): String<br>окружение. |
| [licenseURL](license-u-r-l.md) | [core]<br>val [licenseURL](license-u-r-l.md): Uri?<br>ссылка на условия оплаты, для Яндекс.Оплат. |
| [merchantInfo](merchant-info.md) | [core]<br>val [merchantInfo](merchant-info.md): [MerchantInfo](../-merchant-info/index.md)?<br>информация о мерчанте, для Яндекс Оплат. |
| [payMethodMarkup](pay-method-markup.md) | [core]<br>val [payMethodMarkup](pay-method-markup.md): [PaymethodMarkup](../-paymethod-markup/index.md)?<br>разметка платежа в Трасте. |
| [total](total.md) | [core]<br>val [total](total.md): String<br>сумма. |
