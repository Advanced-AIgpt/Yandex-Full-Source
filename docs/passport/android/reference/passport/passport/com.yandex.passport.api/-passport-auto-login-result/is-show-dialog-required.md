//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAutoLoginResult](index.md)/[isShowDialogRequired](is-show-dialog-required.md)

# isShowDialogRequired

[passport]\
abstract fun [isShowDialogRequired](is-show-dialog-required.md)(): Boolean

В случаее если произошел автологин через smartlock, то гугл уже отобразил диалог об автологине. Иначе требуется показать наш диалог.

#### Return

true - если требуется показать диалог [createAutoLoginIntent](../-passport-api/create-auto-login-intent.md)
