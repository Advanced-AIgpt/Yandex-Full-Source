# ApplePayDelegate

Делегат для работы с контроллером ApplePay.

``` swift
public protocol ApplePayDelegate: AnyObject 
```

## Inheritance

`AnyObject`

## Requirements

### applePayServiceDidRequestPresenting(requestsPresenting:​)

Запрос на отображение контроллера ApplePay.

``` swift
func applePayServiceDidRequestPresenting(requestsPresenting controller: UIViewController) -> Bool
```

#### Parameters

  - controller: контроллер.

#### Returns

удалось ли запустить отображение контроллера.

### applePayServiceDidDismissViewController(error:​)

Контроллер ApplePay будет закрыт.

``` swift
func applePayServiceDidDismissViewController(error: PaymentKitError?)
```

#### Parameters

  - error: ошибка работы с ApplePay, если не `nil`
