//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createBindPhoneIntent](create-bind-phone-intent.md)

# createBindPhoneIntent

[passport]\

@CheckResult

@AnyThread

abstract fun [createBindPhoneIntent](create-bind-phone-intent.md)(@NonNullcontext: Context, @NonNullproperties: [PassportBindPhoneProperties](../-passport-bind-phone-properties/index.md)): Intent

Создаёт интент с экраном привязки номера телефона. 

 В случае если биндинг успешен, вернётся результат RESULT_OK и extra [EXTRA_PHONE_NUMBER](../-passport/-e-x-t-r-a_-p-h-o-n-e_-n-u-m-b-e-r.md) типа String

#### Return

интент для запуска Activity привязки номера

## Parameters

passport

| | |
|---|---|
| context | контекст приложения |
| properties | параметры для запуска привязки |
