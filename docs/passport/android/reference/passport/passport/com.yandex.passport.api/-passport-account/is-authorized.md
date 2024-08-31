//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccount](index.md)/[isAuthorized](is-authorized.md)

# isAuthorized

[passport]\
abstract fun [isAuthorized](is-authorized.md)(): Boolean

true – на устройстве присутствует авторизационный токен (&quot;мастер&quot; токен), по которому можно попытаться получить [токен приложения](../-passport-token/index.md) (&quot;клиентский&quot; токен). Обычно это означает, что пользователь авторизован, но обращение к серверу за токеном приложения может показать, что он уже отозван.
