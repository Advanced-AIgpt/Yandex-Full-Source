# Payment.Method

Метод оплаты.

``` swift
public enum Method: Equatable 
```

## Inheritance

`Equatable`

## Enumeration Cases

### `card`

``` swift
case card(id: Card.Id, system: Card.PaymentSystem, account: String, bankName: BankName, familyInfo: FamilyInfo?)
```

### `yandexBank`

``` swift
case yandexBank(id: String, isOwner: Bool)
```

### `newCard`

``` swift
case newCard
```

### `applePay`

``` swift
case applePay
```

### `sbp`

``` swift
case sbp
```

### `cash`

``` swift
case cash
```

### `tinkoffCredit`

``` swift
case tinkoffCredit
```
