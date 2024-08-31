//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[onPushMessageReceived](on-push-message-received.md)

# onPushMessageReceived

[passport]\
abstract suspend fun [onPushMessageReceived](on-push-message-received.md)(from: String, data: Bundle): Result&lt;Boolean&gt;

Метод требуется вызывать при получении любого push сообщения. Если push сообщение будет предназначено для AM, то метод вернет true. В приложении такое сообщение не должно обрабатываться.

#### Return

true если push сообщение адресовано библиотеке Паспорт

## Parameters

passport

| | |
|---|---|
| from | значение from, полученное в push сообщении |
| data | данные push сообщения |
