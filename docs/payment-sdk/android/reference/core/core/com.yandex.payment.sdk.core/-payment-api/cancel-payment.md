//[core](../../../index.md)/[com.yandex.payment.sdk.core](../index.md)/[PaymentApi](index.md)/[cancelPayment](cancel-payment.md)

# cancelPayment

[core]\

@MainThread()

abstract fun [cancelPayment](cancel-payment.md)()

Отменить работу с текущим платежом.

{% note info %}

Работает аналогично вызову [Payment.cancel](-payment/cancel.md) - не отменяет корзину в самом Трасте, только перестает работать с ней на клиенте.

{% endnote %}
