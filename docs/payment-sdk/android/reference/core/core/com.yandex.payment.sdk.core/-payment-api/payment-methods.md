//[core](../../../index.md)/[com.yandex.payment.sdk.core](../index.md)/[PaymentApi](index.md)/[paymentMethods](payment-methods.md)

# paymentMethods

[core]\

@WorkerThread()

abstract fun [paymentMethods](payment-methods.md)(): [Result](../../com.yandex.payment.sdk.core.data/-result/index.md)<List<[PaymentMethod](../../com.yandex.payment.sdk.core.data/-payment-method/index.md)>>

Получить список методов оплаты, доступный данному пользователю на данном сервисе. **Важно:** для конкретных корзин могут быть доступны особые методы оплаты, если они были заданы при создании и их можно будет получить только в [Payment.methods](-payment/methods.md).

#### Return

список методов.
