//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[Payment](index.md)/[cancel](cancel.md)

# cancel

[core]\

@MainThread()

abstract fun [cancel](cancel.md)()

Отменить работу с платежом.

{% note warning %}

<ul><li>Не отменяет корзину в самом Трасте, только перестает работать с ней на клиенте.</li><li>Если был вызван [pay](pay.md), то просто будет прекращён поллинг статуса, сама оплата при этом может успеть пройти.</li></ul>

{% endnote %}
