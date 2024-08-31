# PaymentCallbacks

Основные коллбеки `PaymentApi`.

``` swift
public protocol PaymentCallbacks 
```

## Requirements

### show3ds(url:​)

Показать 3ds.

``` swift
func show3ds(url: URL)
```

#### Parameters

  - url: адрес 3ds-страницы.

### hide3ds()

Скрыть 3ds.

``` swift
func hide3ds()
```

### cvvRequested()

Необходимо дослать CVV/CVC код.

``` swift
func cvvRequested()
```

### newCardDataRequested()

Необходимо дослать данные новой карты.

``` swift
func newCardDataRequested()
```
