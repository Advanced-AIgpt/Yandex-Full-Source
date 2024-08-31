//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportUid](../index.md)/[Factory](index.md)/[from](from.md)

# from

[passport]\

@NonNull

open fun [from](from.md)(value: Long): [PassportUid](../index.md)

 Фабричный метод, возвращающий ненулевой объект паспортного uid-а [PassportUid](../index.md) по его числовому значению. 

 Требует только числовое значение uid-а, а окружение старается определить автоматически с помощью нехитрых эвристик. Фактически, однозначно может определить только два внутренних окружения - [PASSPORT_ENVIRONMENT_TEAM_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-p-r-o-d-u-c-t-i-o-n.md) и [PASSPORT_ENVIRONMENT_TEAM_TESTING](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-t-e-s-t-i-n-g.md) - а все остальные uid-ы считает находящимися внутри боевого внешнего окружения [PASSPORT_ENVIRONMENT_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md). Поэтому, если вы умеете работать с тестовым внешним окружением [PASSPORT_ENVIRONMENT_TESTING](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-s-t-i-n-g.md) или релиз-кандидатом внешнего окружения [PASSPORT_ENVIRONMENT_RC](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-r-c.md), то лучше использовать соседний фабричный метод [from](from.md). 

 Рекомендуется для использования приложениями, работающими только с боевым внешним [PASSPORT_ENVIRONMENT_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md) или только с боевым внутренним [PASSPORT_ENVIRONMENT_TEAM_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-p-r-o-d-u-c-t-i-o-n.md) окружением, а также при миграции со старых версий библиотек (с 5.x/4.x) на текущую (6.x). 

#### Return

Ненулевой объект паспортного uid-а [PassportUid](../index.md).

## See also

passport

| | |
|---|---|
| [from(PassportEnvironment, long)](from.md) |  |
| [com.yandex.passport.api.PassportUid](../index.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../../-passport-environment/index.md) |  |
| [com.yandex.passport.api.Passport](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-r-c.md) |  |

## Parameters

passport

| | |
|---|---|
| value | Числовое положительное значение uid-а. |

[passport]\

@NonNull

open fun [from](from.md)(@NonNullenvironment: [PassportEnvironment](../../-passport-environment/index.md), value: Long): [PassportUid](../index.md)

 Фабричный метод, возвращающий ненулевой объект паспортного uid-а [PassportUid](../index.md) по его числовому значению и явно указанному окружению [PassportEnvironment](../../-passport-environment/index.md). 

 Рекомендуется для использования приложениями, работающими с несколькими паспортными окружениями, а также при неверной работе соседнего фабричного метода [from](from.md), использующего эвристику для автоматического определения окружения по значению uid-а. 

#### Return

Ненулевой объект паспортного uid-а [PassportUid](../index.md).

## See also

passport

| | |
|---|---|
| [from(long)](from.md) |  |
| [com.yandex.passport.api.PassportUid](../index.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../../-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportEnvironment.Factory](../../../../passport/com.yandex.passport.api/-passport-environment/-factory/from.md) |  |

## Parameters

passport

| | |
|---|---|
| environment | Объект паспортного окружения [PassportEnvironment](../../-passport-environment/index.md). Может быть получен с помощью фабричного метода [from](../../../../passport/com.yandex.passport.api/-passport-environment/-factory/from.md). |
| value | Числовое положительное значение uid-а. |
