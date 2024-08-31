# TinkoffSubmitFormDelegate

Протокол обработки состояния страницы с кредитами.

``` swift
public protocol TinkoffSubmitFormDelegate: AnyObject 
```

## Inheritance

`AnyObject`

## Requirements

### onSubmitTinkoffCreditForm(\_:​)

Новое состояние страницы.

``` swift
func onSubmitTinkoffCreditForm(_ state: TinkoffFormState)
```

#### Parameters

  - state: состояние.
