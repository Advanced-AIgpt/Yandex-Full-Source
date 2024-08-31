//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportFilter](index.md)

# PassportFilter

[passport]\
interface [PassportFilter](index.md)

Contains a filter of accounts by their type.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportEnvironment](../-passport-environment/index.md) |  |
| [com.yandex.passport.api.Passport](../-passport/create-passport-filter-builder.md) |  |

## Types

| Name | Summary |
|---|---|
| [Builder](-builder/index.md) | [passport]<br>interface [Builder](-builder/index.md) : [PassportFilter](index.md) |

## Properties

| Name | Summary |
|---|---|
| [excludeLite](exclude-lite.md) | [passport]<br>abstract val [excludeLite](exclude-lite.md): Boolean<br>Не использовать &quot;lite&quot; аккаунты.<br></br> |
| [excludeSocial](exclude-social.md) | [passport]<br>abstract val [excludeSocial](exclude-social.md): Boolean<br>Не использовать социальные аккаунты (соцсети).<br></br> |
| [includeMailish](include-mailish.md) | [passport]<br>abstract val [includeMailish](include-mailish.md): Boolean<br>Использовать также &quot;мейлиш&quot; аккаунты.<br></br> |
| [includeMusicPhonish](include-music-phonish.md) | [passport]<br>abstract val [includeMusicPhonish](include-music-phonish.md): Boolean<br>Использовать также &quot;фониш&quot; аккаунты с действующей подпиской на Музыку или Плюс.<br></br><u>Не требует</u> использования .includePhonish |
| [includePhonish](include-phonish.md) | [passport]<br>abstract val [includePhonish](include-phonish.md): Boolean<br>Использовать также &quot;фониш&quot; аккаунты.<br></br> |
| [onlyPdd](only-pdd.md) | [passport]<br>abstract val [onlyPdd](only-pdd.md): Boolean<br>Использовать только ПДД аккаунты. |
| [onlyPhonish](only-phonish.md) | [passport]<br>abstract val [onlyPhonish](only-phonish.md): Boolean<br>Использовать только &quot;фониш&quot; аккаунты. |
| [primaryEnvironment](primary-environment.md) | [passport]<br>abstract val [primaryEnvironment](primary-environment.md): [PassportEnvironment](../-passport-environment/index.md)<br>Определить главное окружение. Обычно это [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md). |
| [secondaryTeamEnvironment](secondary-team-environment.md) | [passport]<br>abstract val [secondaryTeamEnvironment](secondary-team-environment.md): [PassportEnvironment](../-passport-environment/index.md)?<br>Определить дополнительное окружение. Обычно это [Passport.PASSPORT_ENVIRONMENT_TEAM_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-p-r-o-d-u-c-t-i-o-n.md), если приложение поддерживает работу с аккаунтами <tt>@yandex-team.tld</tt>. |

## Inheritors

| Name |
|---|
| [Builder](-builder/index.md) |
