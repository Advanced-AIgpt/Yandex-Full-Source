//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createAutoLoginIntent](create-auto-login-intent.md)

# createAutoLoginIntent

[passport]\

@CheckResult

@NonNull

@AnyThread

abstract fun [createAutoLoginIntent](create-auto-login-intent.md)(@NonNullcontext: Context, @NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullautoLoginProperties: [PassportAutoLoginProperties](../-passport-auto-login-properties/index.md)): Intent

 Создает Intent для отображения информации об автологине 

 Пользователь может отменить автологин, нажав соответствующую кнопку 

 Intent нужно запускать через startActivityForResult. В onActivityResult придет результат: 

RESULT_OK - если пользователь нажал &quot;Продолжить&quot; или ничего не делал в течении 5 секундRESULT_CANCELED - если пользователь нажал &quot;Выйти&quot;

 При отображении учитывается [setTheme](../../../../passport/passport/com.yandex.passport.api/-passport-auto-login-properties/-builder/set-theme.md)

#### Return

Intent для открытия новой активити через startActivityForResult

## Parameters

passport

| | |
|---|---|
| context | контекст приложения |
| uid | PassportUid аккаунта для автологина |
| autoLoginProperties | параметры для автологина |
