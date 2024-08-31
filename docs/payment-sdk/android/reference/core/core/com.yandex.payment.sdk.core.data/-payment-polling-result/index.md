//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[PaymentPollingResult](index.md)

# PaymentPollingResult

[core]\
enum [PaymentPollingResult](index.md) : Enum<[PaymentPollingResult](index.md)> 

Результат поллинга платежа. Для всех методов кроме Тинькофф кредита это всегда будет [SUCCESS](-s-u-c-c-e-s-s/index.md). Для кредитов возможны оба статуса, в зависимости от одобрения кредита.

## Entries

| | |
|---|---|
| [WAIT_FOR_PROCESSING](-w-a-i-t_-f-o-r_-p-r-o-c-e-s-s-i-n-g/index.md) | [core]<br>[WAIT_FOR_PROCESSING](-w-a-i-t_-f-o-r_-p-r-o-c-e-s-s-i-n-g/index.md)()<br>Платеж ожидает обработки. |
| [SUCCESS](-s-u-c-c-e-s-s/index.md) | [core]<br>[SUCCESS](-s-u-c-c-e-s-s/index.md)()<br>Платеж успешно проведен. |
