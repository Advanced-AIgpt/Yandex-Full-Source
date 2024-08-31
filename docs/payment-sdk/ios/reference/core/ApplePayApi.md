# ApplePayApi

Протокол для работы с ApplePay. Содержит вспомогательные методы, не работающие с самими платежами в Трасте.

``` swift
public protocol ApplePayApi 
```

## Requirements

### makeApplePayToken(order:​completion:​)

Получить от ApplePay токен для заказа. Может быть полезно для привязки платежей и досписаний.

``` swift
func makeApplePayToken(order: Order, completion: @escaping (PaymentKitResult<String>) -> Void)
```

#### Parameters

  - order: параметры заказа.
  - completion: коллбек с токеном.

### bindApplePayToken(appleToken:​orderTag:​token:​completion:​)

Привязать токен ApplePay в Trust.

``` swift
func bindApplePayToken(appleToken: String, orderTag: Order.Tag?, token: Payment.Token?, completion: @escaping (PaymentKitResult<BoundApplePayToken>) -> Void)
```

#### Parameters

  - appleToken: токен ApplePay.
  - orderTag: тэг заказа.
  - token: токен корзины.
  - completion: коллбек с результатом.

### isApplePayAvailable

Доступен ли на устройстве ApplePay для оплаты. Проверяет только само устройство, на самом сервисе оплата через ApplePay может быть недоступна и нужно использовать `PaymentApi.paymentMethods`.

``` swift
var isApplePayAvailable: Bool 
```
