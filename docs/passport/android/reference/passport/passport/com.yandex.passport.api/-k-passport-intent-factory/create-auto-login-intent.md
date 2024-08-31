//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createAutoLoginIntent](create-auto-login-intent.md)

# createAutoLoginIntent

[passport]\
abstract fun [createAutoLoginIntent](create-auto-login-intent.md)(context: Context, uid: [PassportUid](../-passport-uid/index.md), autoLoginProperties: [PassportAutoLoginProperties](../-passport-auto-login-properties/index.md)): Intent

Создает Intent для отображения информации об автологине

Пользователь может отменить автологин, нажав соответствующую кнопку

Intent нужно запускать через android.app.Activity.startActivityForResult. В android.app.Activity.onActivityResult придет результат:

<ui>
* [android.app.Activity.RESULT_OK] - если пользователь нажал "Продолжить" или ничего не делал в течении 5 секунд
* [android.app.Activity.RESULT_CANCELED] - если пользователь нажал "Выйти"
</ui>

При отображении учитывается [PassportAutoLoginProperties.Builder.setTheme](../-passport-auto-login-properties/-builder/set-theme.md)

#### Return

Intent для открытия новой активити через startActivityForResult

## Parameters

passport

| | |
|---|---|
| context | контекст приложения |
| uid | PassportUid аккаунта для автологина |
| autoLoginProperties | параметры для автологина |
