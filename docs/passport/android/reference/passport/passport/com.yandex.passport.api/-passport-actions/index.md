//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportActions](index.md)

# PassportActions

[passport]\
interface [PassportActions](index.md)

## Properties

| Name | Summary |
|---|---|
| [CLIENT_ACTION_ACCOUNT_ADDED](-c-l-i-e-n-t_-a-c-t-i-o-n_-a-c-c-o-u-n-t_-a-d-d-e-d.md) | [passport]<br>val [CLIENT_ACTION_ACCOUNT_ADDED](-c-l-i-e-n-t_-a-c-t-i-o-n_-a-c-c-o-u-n-t_-a-d-d-e-d.md): String<br>Аккаунт был добавлен android.content.Intent содержит объект [PassportUid](../-passport-uid/index.md) - uid добавленного аккаунта |
| [CLIENT_ACTION_ACCOUNT_REMOVED](-c-l-i-e-n-t_-a-c-t-i-o-n_-a-c-c-o-u-n-t_-r-e-m-o-v-e-d.md) | [passport]<br>val [CLIENT_ACTION_ACCOUNT_REMOVED](-c-l-i-e-n-t_-a-c-t-i-o-n_-a-c-c-o-u-n-t_-r-e-m-o-v-e-d.md): String<br>Аккаунт был удалён android.content.Intent содержит объект [PassportUid](../-passport-uid/index.md) - uid удаленного аккаунта |
| [CLIENT_ACTION_PREFIX](-c-l-i-e-n-t_-a-c-t-i-o-n_-p-r-e-f-i-x.md) | [passport]<br>val [CLIENT_ACTION_PREFIX](-c-l-i-e-n-t_-a-c-t-i-o-n_-p-r-e-f-i-x.md): String<br>Все события в этом интерфейсе всегда должны начинаться с этой строки |
| [CLIENT_ACTION_SEND_AUTH_TO_TRACK](-c-l-i-e-n-t_-a-c-t-i-o-n_-s-e-n-d_-a-u-t-h_-t-o_-t-r-a-c-k.md) | [passport]<br>@Deprecated<br>~~val~~ [~~CLIENT_ACTION_SEND_AUTH_TO_TRACK~~](-c-l-i-e-n-t_-a-c-t-i-o-n_-s-e-n-d_-a-u-t-h_-t-o_-t-r-a-c-k.md)~~:~~ String<br>Action для запуска активити для авторизации устройства на котором был получет trackId. |
| [CLIENT_ACTION_TOKEN_CHANGED](-c-l-i-e-n-t_-a-c-t-i-o-n_-t-o-k-e-n_-c-h-a-n-g-e-d.md) | [passport]<br>val [CLIENT_ACTION_TOKEN_CHANGED](-c-l-i-e-n-t_-a-c-t-i-o-n_-t-o-k-e-n_-c-h-a-n-g-e-d.md): String<br>У аккаунта был изменился master token (пользователь залогинился в существующий аккаунт) android.content.Intent содержит объект [PassportUid](../-passport-uid/index.md) - uid измененного аккаунта |
| [EXTRA_ENVIRONMENT](-e-x-t-r-a_-e-n-v-i-r-o-n-m-e-n-t.md) | [passport]<br>val [EXTRA_ENVIRONMENT](-e-x-t-r-a_-e-n-v-i-r-o-n-m-e-n-t.md): String |
| [EXTRA_PAYMENT_AUTH_CONTEXT_ID](-e-x-t-r-a_-p-a-y-m-e-n-t_-a-u-t-h_-c-o-n-t-e-x-t_-i-d.md) | [passport]<br>val [EXTRA_PAYMENT_AUTH_CONTEXT_ID](-e-x-t-r-a_-p-a-y-m-e-n-t_-a-u-t-h_-c-o-n-t-e-x-t_-i-d.md): String |
| [EXTRA_PAYMENT_AUTH_URL](-e-x-t-r-a_-p-a-y-m-e-n-t_-a-u-t-h_-u-r-l.md) | [passport]<br>val [EXTRA_PAYMENT_AUTH_URL](-e-x-t-r-a_-p-a-y-m-e-n-t_-a-u-t-h_-u-r-l.md): String |
| [EXTRA_UID](-e-x-t-r-a_-u-i-d.md) | [passport]<br>val [EXTRA_UID](-e-x-t-r-a_-u-i-d.md): String |
| [EXTRA_URI](-e-x-t-r-a_-u-r-i.md) | [passport]<br>val [EXTRA_URI](-e-x-t-r-a_-u-r-i.md): String |
| [MONEY_ACTION_PAYMENT_AUTHORIZATION](-m-o-n-e-y_-a-c-t-i-o-n_-p-a-y-m-e-n-t_-a-u-t-h-o-r-i-z-a-t-i-o-n.md) | [passport]<br>val [MONEY_ACTION_PAYMENT_AUTHORIZATION](-m-o-n-e-y_-a-c-t-i-o-n_-p-a-y-m-e-n-t_-a-u-t-h-o-r-i-z-a-t-i-o-n.md): String<br>Action для запуска платежной авторизации из AuthSDK |
