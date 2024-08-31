# PaymentSdkCellProps.CellType

Типы ячеек.

``` swift
public enum CellType 
```

## Enumeration Cases

### `regular`

Обычная ячейка.

``` swift
case regular
```

### `cvn`

Ячейка со вводом CVC/CVV

``` swift
case cvn(paymentSystem: Payment.Card.PaymentSystem)
```

### `expanded`

Ячейка со стрелкой

``` swift
case expanded
```
