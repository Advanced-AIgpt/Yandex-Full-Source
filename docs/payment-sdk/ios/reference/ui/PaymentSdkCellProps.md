# PaymentSdkCellProps

Ячейки для отображения методов из PaymentSDK&

``` swift
public struct PaymentSdkCellProps: SelectCellProps 
```

## Inheritance

[`SelectCellProps`](/SelectCellProps)

## Initializers

### `init(method:title:subtitle:leftIcon:rightIcon:type:)`

``` swift
public init(method: Payment.Method, title: String, subtitle: String?, leftIcon: UIImage?, rightIcon: UIImage?, type: CellType) 
```

## Methods

### `propsFrom(methods:needCvn:mode:isLightTheme:)`

Порождающий метод для свойств ячеек таблицы.

``` swift
static func propsFrom(methods: [Payment.Method], needCvn: (Payment.Method) -> Bool, mode: PaymentCellIconsMode, isLightTheme: Bool) -> [PaymentSdkCellProps] 
```

#### Parameters

  - methods: методы оплаты.
  - needCvn: коллбек для проверки необходимости CVC/CVV в каком-либо методе.
  - mode: режим отображения иконок.
  - isLightTheme: использовать ли светлую тему.

#### Returns

массив свойств для ячеек.
