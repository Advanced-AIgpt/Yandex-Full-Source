# PaymentSdkResources

Получение различных общих ресурсов.

``` swift
public class PaymentSdkResources 
```

## Methods

### `getCardPaymentSystemIcon(_:isLightTheme:)`

Получить иконку платёжной системы.

``` swift
public static func getCardPaymentSystemIcon(_ system: Payment.Card.PaymentSystem, isLightTheme: Bool) -> UIImage? 
```

#### Parameters

  - system: платёжная система.
  - isLightTheme: для `true` будет возвращена иконка для светлой темы, иначе - для темной.

#### Returns

иконка либо nil, если для такой платёжной системы она отсутствует в PaymentSDK

### `getPaymentMethodIcon(_:isLightTheme:)`

Получить иконку для метода оплаты.

``` swift
public static func getPaymentMethodIcon(_ method: Payment.Method, isLightTheme: Bool) -> UIImage? 
```

#### Parameters

  - method: платёжный метод.
  - isLightTheme: для `true` будет возвращена иконка для светлой темы, иначе - для темной.

#### Returns

иконка либо nil, если для такого метода она отсутствует в PaymentSDK

### `getBankIconForName(_:isLightTheme:)`

Получить иконку банка.

``` swift
public static func getBankIconForName(_ name: Payment.BankName, isLightTheme: Bool) -> UIImage 
```

#### Parameters

  - name: название банка.
  - isLightTheme: для `true` будет возвращена иконка для светлой темы, иначе - для темной.

#### Returns

иконка
