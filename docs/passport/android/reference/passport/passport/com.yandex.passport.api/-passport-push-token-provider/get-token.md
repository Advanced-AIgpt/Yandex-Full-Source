//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportPushTokenProvider](index.md)/[getToken](get-token.md)

# getToken

[passport]\

@WorkerThread

@CheckResult

abstract fun [getToken](get-token.md)(@NonNullsenderId: String): String

Метод должен быть реализован в паспортом процессе. Если вы используете GCM, то реализация следующая: InstanceID.getToken(gcmSenderId, GoogleCloudMessaging.INSTANCE_ID_SCOPE); Если вы используете FCM, то реализация следующая: FirebaseInstanceId.getInstance().getToken(gcmSenderId, FirebaseMessaging.INSTANCE_ID_SCOPE)

#### Return

токен для gcm

## Parameters

passport

| | |
|---|---|
| senderId | - gcm of fcm sender id |
