//[ui](../../../index.md)/[com.yandex.payment.sdk.ui](../index.md)/[CvnInput](index.md)/[setOnReadyListener](set-on-ready-listener.md)

# setOnReadyListener

[ui]\
abstract fun [setOnReadyListener](set-on-ready-listener.md)(listener: (Boolean) -> Unit?)

Установить листенер корректности введенного кода. Листенер будет вызываться при смене состояний. **Важно:** под корректностью подразумевается правильная длина и наличие только цифр, соответствие же кода написанному на карте будет проверено только во время оплаты на бэкенде в контуре PCI DSS.

## Parameters

ui

| | |
|---|---|
| listener | листенер. |
