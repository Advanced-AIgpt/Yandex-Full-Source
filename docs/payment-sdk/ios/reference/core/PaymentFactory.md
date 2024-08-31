# PaymentFactory

Фабрика для инициализации.

``` swift
public final class PaymentFactory 
```

## Methods

### `getInstance()`

Получить инстанс фабрики.

``` swift
public static func getInstance() -> PaymentFactory 
```

#### Returns

фабрика.

### `setPaymentKitMode(_:)`

Задать окружение работы.

``` swift
@discardableResult
    public static func setPaymentKitMode(_ value: PaymentKitMode) -> PaymentFactory.Type 
```

#### Parameters

  - value: окружение.

#### Returns

класс фабрики (для соединения вызовов).

### `setConsoleLoggingMode(_:)`

Задать режим логирования

``` swift
@discardableResult
    public static func setConsoleLoggingMode(_ value: ConsoleLoggingMode) -> PaymentFactory.Type 
```

#### Parameters

  - value: режим логирования

#### Returns

класс фабрики (для соединения вызовов).

### `setMetricaMode(_:)`

Задать режим работы Метрики

``` swift
@discardableResult
    public static func setMetricaMode(_ value: Metrica.Mode) -> PaymentFactory.Type 
```

#### Parameters

  - value: режим работы Метрики.

#### Returns

класс фабрики (для соединения вызовов).

### `setAuthExchanger(handler:)`

Задать реализацию для обмена скоупа токенов в Паспорте.

``` swift
@discardableResult
    public static func setAuthExchanger(handler: PaymentKitAuthExchanger) -> PaymentFactory.Type 
```

#### Parameters

  - handler: хэндлер обмена скоупов.

#### Returns

класс фабрики (для соединения вызовов).

### `setAuthChallengeDisposition(_:)`

Устанавливает тип проверки соединения:​ дефолтный, sslPining, YandexCA revoke MOBILECOM-196

``` swift
@discardableResult
    public static func setAuthChallengeDisposition(_ value: AuthChallengeDisposition) -> PaymentFactory.Type 
```

> 

#### Parameters

  - value: тип проверки.

#### Returns

класс фабрики (для соединения вызовов).

### `initialize()`

Инициализировать фабрику.

``` swift
@discardableResult
    public static func initialize() -> PaymentFactory 
```

#### Returns

инициализированный объект фабрики.

### `setupAsync(completion:)`

``` swift
@available(*, deprecated, message: "Do not needed anymore")
    public func setupAsync(completion: (() -> Void)? = nil) 
```

### `setupAsync(uuid:completion:)`

``` swift
@available(*, deprecated, message: "Do not needed anymore")
    public func setupAsync(uuid: String, completion: (() -> Void)? = nil) 
```

### `makePaymentApiBuilder(payer:merchant:callbacks:)`

Вернуть билдер для создания `PaymentApi`

``` swift
public func makePaymentApiBuilder(payer: Payment.Payer,
                                      merchant: Payment.Merchant,
                                      callbacks: PaymentCallbacks) -> PaymentApiBuilder 
```

#### Parameters

  - payer: информация о пользователе.
  - merchant: информация о сервисе.
  - callbacks: коллбеки.

#### Returns
