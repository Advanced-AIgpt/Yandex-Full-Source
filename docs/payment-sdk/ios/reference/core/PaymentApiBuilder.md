# PaymentApiBuilder

Билдер для создания объекта `PaymentApi`.

``` swift
public class PaymentApiBuilder 
```

## Methods

### `applePay(_:)`

настройки ApplePay

``` swift
public func applePay(_ applePay: Payment.ApplePay?) -> PaymentApiBuilder 
```

### `paymentMethodsFilter(_:)`

Фильтр доступных методов оплаты.

``` swift
public func paymentMethodsFilter(_ paymentMethodsFilter: Payment.MethodsFilter) -> PaymentApiBuilder 
```

### `appInfo(_:)`

Информация о турбоаппе ПП/Браузера.

``` swift
public func appInfo(_ appInfo: Payment.AppInfo) -> PaymentApiBuilder 
```

### `forceCVV(_:)`

Всегда запрашивать CVV/CVC код при оплате картой.

``` swift
public func forceCVV(_ forceCVV: Bool) -> PaymentApiBuilder 
```

### `authExchanger(_:)`

Реализация обмена скоупа токенов в Паспорте.

``` swift
public func authExchanger(_ authExchanger: PaymentKitAuthExchanger?) -> PaymentApiBuilder 
```

### `applePayDelegate(_:)`

Делегат для контроллера ApplePay

``` swift
public func applePayDelegate(_ applePayDelegate: ApplePayDelegate?) -> PaymentApiBuilder 
```

### `passportToken(_:)`

Паспортный токен для вебавторизации. Нужен только для CrowdTesting.

``` swift
public func passportToken(_ passportToken: String?) -> PaymentApiBuilder 
```

### `enableCashPayments(_:)`

Разрешить наличные как метод оплаты.

``` swift
public func enableCashPayments(_ enableCashPayments: Bool) -> PaymentApiBuilder 
```

### `regionId(_:)`

Регион пользователя.

``` swift
public func regionId(_ regionId: Int32) -> PaymentApiBuilder 
```

### `build()`

Собрать `PaymentApi`

``` swift
public func build() -> PaymentApi 
```
