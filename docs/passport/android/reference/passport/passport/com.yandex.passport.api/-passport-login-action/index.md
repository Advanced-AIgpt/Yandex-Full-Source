//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportLoginAction](index.md)

# PassportLoginAction

[passport]\
enum [PassportLoginAction](index.md) : Enum&lt;[PassportLoginAction](index.md)&gt; 

Определяет что произошло с аккаунтом перед тем как его вернули приложению.

## Entries

| | |
|---|---|
| [REG_NEO_PHONISH](-r-e-g_-n-e-o_-p-h-o-n-i-s-h/index.md) | [passport]<br>[REG_NEO_PHONISH](-r-e-g_-n-e-o_-p-h-o-n-i-s-h/index.md)()<br>Зарегистрировали неофониша |
| [LOGIN_RESTORE](-l-o-g-i-n_-r-e-s-t-o-r-e/index.md) | [passport]<br>[LOGIN_RESTORE](-l-o-g-i-n_-r-e-s-t-o-r-e/index.md)()<br>Авторизовались после восстановления логина |
| [SMS](-s-m-s/index.md) | [passport]<br>[SMS](-s-m-s/index.md)()<br>Авторизовались по SMS |
| [QR_ON_TV](-q-r_-o-n_-t-v/index.md) | [passport]<br>[QR_ON_TV](-q-r_-o-n_-t-v/index.md)()<br>Авторизовались по QR коду в Android TV |
| [TRACK_ID](-t-r-a-c-k_-i-d/index.md) | [passport]<br>[TRACK_ID](-t-r-a-c-k_-i-d/index.md)()<br>Авторизовались по track_id (вероятно по QR коду) |
| [MAGIC_LINK](-m-a-g-i-c_-l-i-n-k/index.md) | [passport]<br>[MAGIC_LINK](-m-a-g-i-c_-l-i-n-k/index.md)()<br>Авторизовались(зарегистрировались) по внешнему magic link |
| [MAILISH_GIMAP](-m-a-i-l-i-s-h_-g-i-m-a-p/index.md) | [passport]<br>[MAILISH_GIMAP](-m-a-i-l-i-s-h_-g-i-m-a-p/index.md)()<br>GIMAP Mailish авторизация |
| [EMPTY](-e-m-p-t-y/index.md) | [passport]<br>[EMPTY](-e-m-p-t-y/index.md)()<br>Авторизационных действий с аккаунтом произведено не было |
| [AUTOLOGIN](-a-u-t-o-l-o-g-i-n/index.md) | [passport]<br>[AUTOLOGIN](-a-u-t-o-l-o-g-i-n/index.md)()<br>Аккаунт добавлен на устройство из автологина из смартлока |
| [PHONISH](-p-h-o-n-i-s-h/index.md) | [passport]<br>[PHONISH](-p-h-o-n-i-s-h/index.md)()<br>Авторизовались(зарегистрировались) по номеру телефона |
| [REGISTRATION](-r-e-g-i-s-t-r-a-t-i-o-n/index.md) | [passport]<br>[REGISTRATION](-r-e-g-i-s-t-r-a-t-i-o-n/index.md)()<br>Зарегистрировали новый портальный аккаунт |
| [TOTP](-t-o-t-p/index.md) | [passport]<br>[TOTP](-t-o-t-p/index.md)()<br>Авторизовались по паролю + totp |
| [PASSWORD](-p-a-s-s-w-o-r-d/index.md) | [passport]<br>[PASSWORD](-p-a-s-s-w-o-r-d/index.md)()<br>Авторизовались по паролю |
| [SOCIAL](-s-o-c-i-a-l/index.md) | [passport]<br>[SOCIAL](-s-o-c-i-a-l/index.md)()<br>Авторизовались через соцсеть |
| [CAROUSEL](-c-a-r-o-u-s-e-l/index.md) | [passport]<br>[CAROUSEL](-c-a-r-o-u-s-e-l/index.md)()<br>Аккаунт выбрали из списка существующих аккаунтов |
