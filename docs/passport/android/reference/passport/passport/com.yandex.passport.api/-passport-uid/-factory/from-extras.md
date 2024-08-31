//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportUid](../index.md)/[Factory](index.md)/[fromExtras](from-extras.md)

# fromExtras

[passport]\

@NonNull

open fun [fromExtras](from-extras.md)(@NonNullbundle: Bundle): [PassportUid](../index.md)

 Фабричный метод, возвращающий ненулевой объект паспортного uid-а [PassportUid](../index.md), содержащийся в Bundle

 Используется для получения [PassportUid](../index.md) из android.content.Intent, пришедшего в BroadcastReceiver 

#### Return

Ненулевой объект паспортного uid-а [PassportUid](../index.md).

## See also

passport

| | |
|---|---|
| [from(PassportEnvironment, long)](from.md) |  |
| [com.yandex.passport.api.PassportUid](../index.md) |  |
| [com.yandex.passport.api.PassportActions](../../-passport-actions/index.md) |  |

## Parameters

passport

| | |
|---|---|
| bundle | содержит [KEY_ENVIRONMENT](-k-e-y_-e-n-v-i-r-o-n-m-e-n-t.md) и [KEY_UID](-k-e-y_-u-i-d.md) |
