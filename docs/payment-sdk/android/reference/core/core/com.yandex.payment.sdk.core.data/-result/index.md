//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[Result](index.md)

# Result

[core]\
sealed class [Result](index.md)<[T](index.md)>

Класс для хранения результата.

## Types

| Name | Summary |
|---|---|
| [Error](-error/index.md) | [core]<br>class [Error](-error/index.md)<[T](-error/index.md)>(**error**: [PaymentKitError](../-payment-kit-error/index.md)) : [Result](index.md)<[T](-error/index.md)> <br>Произошла ошибка. |
| [Success](-success/index.md) | [core]<br>class [Success](-success/index.md)<[T](-success/index.md)>(**value**: [T](-success/index.md)) : [Result](index.md)<[T](-success/index.md)> <br>Операция успешна. |

## Inheritors

| Name |
|---|
| [Result](-success/index.md) |
| [Result](-error/index.md) |
