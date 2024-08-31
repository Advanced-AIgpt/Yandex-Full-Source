//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportEnvironment](index.md)

# KPassportEnvironment

[passport]\
enum [KPassportEnvironment](index.md) : Enum&lt;[KPassportEnvironment](index.md)&gt; , [PassportEnvironment](../-passport-environment/index.md)

Describes the passport environment in enum-way.

At the moment, the library supports several different environments, each of which corresponds to a pre-initialized static object.

Usually applications do not know about any additional environments and work only in the context of a combat external environment. That is, in most situations it can be considered default, and if necessary, explicit instructions (for example, when creating the [PassportUid](../-passport-uid/index.md) object, if you only have a numeric uid value, but you know for sure that it is from the combat environment) you can use [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md).

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportEnvironment](../-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportUid](../-passport-uid/index.md) |  |

## Entries

| | |
|---|---|
| [RC](-r-c/index.md) | [passport]<br>[RC](-r-c/index.md)(Environment.RC) |
| [TEAM_TESTING](-t-e-a-m_-t-e-s-t-i-n-g/index.md) | [passport]<br>[TEAM_TESTING](-t-e-a-m_-t-e-s-t-i-n-g/index.md)(Environment.TEAM_TESTING) |
| [TESTING](-t-e-s-t-i-n-g/index.md) | [passport]<br>[TESTING](-t-e-s-t-i-n-g/index.md)(Environment.TESTING) |
| [TEAM_PRODUCTION](-t-e-a-m_-p-r-o-d-u-c-t-i-o-n/index.md) | [passport]<br>[TEAM_PRODUCTION](-t-e-a-m_-p-r-o-d-u-c-t-i-o-n/index.md)(Environment.TEAM_PRODUCTION) |
| [PRODUCTION](-p-r-o-d-u-c-t-i-o-n/index.md) | [passport]<br>[PRODUCTION](-p-r-o-d-u-c-t-i-o-n/index.md)(Environment.PRODUCTION) |

## Types

| Name | Summary |
|---|---|
| [Companion](-companion/index.md) | [passport]<br>object [Companion](-companion/index.md) |

## Properties

| Name | Summary |
|---|---|
| [integer](integer.md) | [passport]<br>open override val [integer](integer.md): Int<br>Возвращает уникальное и постоянное число, однозначно идентифицирующее это паспортное окружение. |
| [passportEnvironment](passport-environment.md) | [passport]<br>val [passportEnvironment](passport-environment.md): [PassportEnvironment](../-passport-environment/index.md) |
