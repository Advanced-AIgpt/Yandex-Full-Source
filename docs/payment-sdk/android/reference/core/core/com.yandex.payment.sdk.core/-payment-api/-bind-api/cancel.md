//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[BindApi](index.md)/[cancel](cancel.md)

# cancel

[core]\

@MainThread()

abstract fun [cancel](cancel.md)()

Отменить процесс привязки или верификации. **Важно:** будет отменен только поллинг, сама карта может успеть привязаться/верифицироваться, если пользователь прошел все челленджи.
