# PaymentPollingResult

Результат поллинга платежа. Для всех методов кроме Тинькофф кредита это всегда будет `success`.
Для кредитов возможны оба статуса, в зависимости от одобрения кредита.

``` swift
public enum PaymentPollingResult 
```

## Enumeration Cases

### `success`

Платеж успешно проведен.

``` swift
case success
```

### `waitForProcessing`

Платеж ожидает обработки. Только для кредитов Тинькофф.

``` swift
case waitForProcessing
```
