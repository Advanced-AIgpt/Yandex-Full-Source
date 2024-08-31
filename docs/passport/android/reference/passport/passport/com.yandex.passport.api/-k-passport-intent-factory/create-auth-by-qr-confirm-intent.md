//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createAuthByQrConfirmIntent](create-auth-by-qr-confirm-intent.md)

# createAuthByQrConfirmIntent

[passport]\
abstract fun [createAuthByQrConfirmIntent](create-auth-by-qr-confirm-intent.md)(uri: Uri): Intent

Возвращает <tt>Intent</tt> для запуска <tt>Activity</tt> для подтверждения входа по QR коду.

## Parameters

passport

| | |
|---|---|
| uri | зашитая в QR universal-линка, которую можно прочитать любым ридером QR и она может открыться в любом приложении Яндекса, которое эту ссылку поддержало. |
