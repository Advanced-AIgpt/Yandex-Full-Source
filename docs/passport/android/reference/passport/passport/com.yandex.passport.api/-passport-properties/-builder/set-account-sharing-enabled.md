//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportProperties](../index.md)/[Builder](index.md)/[setAccountSharingEnabled](set-account-sharing-enabled.md)

# setAccountSharingEnabled

[passport]\
abstract fun [setAccountSharingEnabled](set-account-sharing-enabled.md)(enabled: Boolean?): [PassportProperties.Builder](index.md)

Включает или отключает обмен аккаунтами с другими accountType. Нельзя использовавать без согласования с командой паспорта.

Если null, то будет использоваться значение эксперимента (по умолчанию отключено) true - аккаунты будут шариться false - аккаунты не будут шариться.
