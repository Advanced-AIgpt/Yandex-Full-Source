//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportUid](../index.md)/[Factory](index.md)

# Factory

[passport]\
open class [Factory](index.md)

## Functions

| Name | Summary |
|---|---|
| [from](from.md) | [passport]<br>@NonNull<br>open fun [from](from.md)(value: Long): [PassportUid](../index.md)<br> Фабричный метод, возвращающий ненулевой объект паспортного uid-а [PassportUid](../index.md) по его числовому значению.<br>[passport]<br>@NonNull<br>open fun [from](from.md)(@NonNullenvironment: [PassportEnvironment](../../-passport-environment/index.md), value: Long): [PassportUid](../index.md)<br> Фабричный метод, возвращающий ненулевой объект паспортного uid-а [PassportUid](../index.md) по его числовому значению и явно указанному окружению [PassportEnvironment](../../-passport-environment/index.md). |
| [fromExtras](from-extras.md) | [passport]<br>@NonNull<br>open fun [fromExtras](from-extras.md)(@NonNullbundle: Bundle): [PassportUid](../index.md)<br> Фабричный метод, возвращающий ненулевой объект паспортного uid-а [PassportUid](../index.md), содержащийся в Bundle Используется для получения [PassportUid](../index.md) из android.content.Intent, пришедшего в BroadcastReceiver |

## Properties

| Name | Summary |
|---|---|
| [KEY_ENVIRONMENT](-k-e-y_-e-n-v-i-r-o-n-m-e-n-t.md) | [passport]<br>val [KEY_ENVIRONMENT](-k-e-y_-e-n-v-i-r-o-n-m-e-n-t.md): String<br>Ключ в Bundle, хранящий значение Environment (int) |
| [KEY_UID](-k-e-y_-u-i-d.md) | [passport]<br>val [KEY_UID](-k-e-y_-u-i-d.md): String<br>Ключ в Bundle, хранящий значение Uid (long) |
