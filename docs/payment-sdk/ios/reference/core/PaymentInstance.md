# PaymentInstance

Интерфейс для работы с оплатой. Создаётся после успешного `PaymentApi.startPayment`.
**Важно:​** в один момент времени может идти работа только с одним платежом, если нужно провести
ещё одну оплату - нужно дождаться завершения предыдущей или сбросить её `cancel`.

``` swift
public protocol PaymentInstance 
```

## Requirements

### pay(method:​overrideUserEmail:​completion:​)

Дослать платёжный метод и начать оплату. Если будет передан метод, недоступный для оплаты в данный момент, коллбек `completion` будет вызван с ошибкой.

``` swift
func pay(method: Payment.Method, overrideUserEmail: String?, completion: @escaping (PaymentKitResult<PaymentPollingResult>) -> Void)
```

#### Parameters

  - method: выбранный платёжный метод.
  - overrideUserEmail: опционально использовать другой email для проведения оплаты.
  - completion: коллбек со статусом платежа.

### shouldShowCvv(cardId:​)

Требуется ли ввод CVV/CVC кода для оплаты данной картой.

``` swift
func shouldShowCvv(cardId: Payment.Card.Id) -> Bool
```

#### Parameters

  - cardId: карта.

#### Returns

нужен ли код.

### methods()

Разрешённые методы оплаты.

``` swift
func methods() -> [Payment.Method]
```

#### Returns

методы оплаты, разрешенные для данной корзины. Могут отличаться от `PaymentApi.paymentMethods`

### settings()

Дополнительные параметры платежа.

``` swift
func settings() -> Payment.Settings
```

#### Returns

параметры.

### setSbpHandler(handler:​)

Задать обработчик для оплаты через Систему Быстрых Платежей.

``` swift
func setSbpHandler(handler: SbpHandler?)
```

#### Parameters

  - handler: обработчик

### cancel()

Отменить работу с платежом.

``` swift
func cancel()
```

{% note warning %}

  - Не отменяет корзину в самом Трасте, только перестает работать с ней на клиенте.

  - Если был вызван `pay`, то просто будет прекращён поллинг статуса, сама оплата при этом может успеть пройти.

{% endnote %}
