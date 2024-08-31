//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportFilter](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportFilter](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportFilter](../index.md) |
| [excludeLite](exclude-lite.md) | [passport]<br>abstract fun [excludeLite](exclude-lite.md)(): [PassportFilter.Builder](index.md) |
| [excludeSocial](exclude-social.md) | [passport]<br>abstract fun [excludeSocial](exclude-social.md)(): [PassportFilter.Builder](index.md) |
| [includeLite](include-lite.md) | [passport]<br>abstract fun [includeLite](include-lite.md)(): [PassportFilter.Builder](index.md) |
| [includeMailish](include-mailish.md) | [passport]<br>abstract fun [includeMailish](include-mailish.md)(): [PassportFilter.Builder](index.md) |
| [includeMusicPhonish](include-music-phonish.md) | [passport]<br>abstract fun [includeMusicPhonish](include-music-phonish.md)(): [PassportFilter.Builder](index.md) |
| [includePhonish](include-phonish.md) | [passport]<br>abstract fun [includePhonish](include-phonish.md)(): [PassportFilter.Builder](index.md) |
| [includeSocial](include-social.md) | [passport]<br>abstract fun [includeSocial](include-social.md)(): [PassportFilter.Builder](index.md) |
| [onlyPdd](only-pdd.md) | [passport]<br>abstract fun [onlyPdd](only-pdd.md)(): [PassportFilter.Builder](index.md) |
| [onlyPhonish](only-phonish.md) | [passport]<br>abstract fun [onlyPhonish](only-phonish.md)(): [PassportFilter.Builder](index.md) |
| [setPrimaryEnvironment](set-primary-environment.md) | [passport]<br>abstract fun [setPrimaryEnvironment](set-primary-environment.md)(primaryEnvironment: [PassportEnvironment](../../-passport-environment/index.md)): [PassportFilter.Builder](index.md) |
| [setSecondaryTeamEnvironment](set-secondary-team-environment.md) | [passport]<br>abstract fun [setSecondaryTeamEnvironment](set-secondary-team-environment.md)(secondaryTeamEnvironment: [PassportEnvironment](../../-passport-environment/index.md)?): [PassportFilter.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [excludeLite](exclude-lite.md) | [passport]<br>abstract override var [excludeLite](exclude-lite.md): Boolean<br>Не использовать &quot;lite&quot; аккаунты.<br></br> |
| [excludeSocial](exclude-social.md) | [passport]<br>abstract override var [excludeSocial](exclude-social.md): Boolean<br>Не использовать социальные аккаунты (соцсети).<br></br> |
| [includeMailish](include-mailish.md) | [passport]<br>abstract override var [includeMailish](include-mailish.md): Boolean<br>Использовать также &quot;мейлиш&quot; аккаунты.<br></br> |
| [includeMusicPhonish](include-music-phonish.md) | [passport]<br>abstract override var [includeMusicPhonish](include-music-phonish.md): Boolean<br>Использовать также &quot;фониш&quot; аккаунты с действующей подпиской на Музыку или Плюс.<br></br><u>Не требует</u> использования .includePhonish |
| [includePhonish](include-phonish.md) | [passport]<br>abstract override var [includePhonish](include-phonish.md): Boolean<br>Использовать также &quot;фониш&quot; аккаунты.<br></br> |
| [onlyPdd](only-pdd.md) | [passport]<br>abstract override var [onlyPdd](only-pdd.md): Boolean<br>Использовать только ПДД аккаунты. |
| [onlyPhonish](only-phonish.md) | [passport]<br>abstract override var [onlyPhonish](only-phonish.md): Boolean<br>Использовать только &quot;фониш&quot; аккаунты. |
| [primaryEnvironment](primary-environment.md) | [passport]<br>abstract override var [primaryEnvironment](primary-environment.md): [KPassportEnvironment](../../-k-passport-environment/index.md)<br>Определить главное окружение. Обычно это [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md). |
| [secondaryTeamEnvironment](secondary-team-environment.md) | [passport]<br>abstract override var [secondaryTeamEnvironment](secondary-team-environment.md): [KPassportEnvironment](../../-k-passport-environment/index.md)?<br>Определить дополнительное окружение. Обычно это [Passport.PASSPORT_ENVIRONMENT_TEAM_PRODUCTION](../../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-p-r-o-d-u-c-t-i-o-n.md), если приложение поддерживает работу с аккаунтами <tt>@yandex-team.tld</tt>. |
