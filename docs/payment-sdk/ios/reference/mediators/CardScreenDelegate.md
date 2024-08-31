# CardScreenDelegate

Протокол для делегата экана ввода данных карт.

``` swift
public protocol CardScreenDelegate: AnyObject 
```

## Inheritance

`AnyObject`

## Requirements

### setState(state:​)

Установить новое состояние экрана.

``` swift
func setState(state: CardScreenState)
```

#### Parameters

  - state: состояние.
