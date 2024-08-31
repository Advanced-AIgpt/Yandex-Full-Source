//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[PaymentCallbacks](index.md)

# PaymentCallbacks

[core]\
interface [PaymentCallbacks](index.md)

Основные коллбеки [com.yandex.payment.sdk.core.PaymentApi](../../com.yandex.payment.sdk.core/-payment-api/index.md)

## Functions

| Name | Summary |
|---|---|
| [cvvRequested](cvv-requested.md) | [core]<br>abstract fun [cvvRequested](cvv-requested.md)()<br>Необходимо дослать CVV/CVC код. |
| [hide3ds](hide3ds.md) | [core]<br>abstract fun [hide3ds](hide3ds.md)()<br>Скрыть 3ds. |
| [newCardDataRequested](new-card-data-requested.md) | [core]<br>abstract fun [newCardDataRequested](new-card-data-requested.md)()<br>Необходимо дослать данные новой карты. |
| [show3ds](show3ds.md) | [core]<br>abstract fun [show3ds](show3ds.md)(url: Uri)<br>Показать 3ds. |
