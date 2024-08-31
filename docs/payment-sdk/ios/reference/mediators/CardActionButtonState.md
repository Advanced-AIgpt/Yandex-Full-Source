# CardActionButtonState

Возможные состояния основной кнопки экрана ввода данных карт.

``` swift
public enum CardActionButtonState 
```

## Enumeration Cases

### `gone`

Кнопка невидима.

``` swift
case gone
```

### `disabled`

Кнопка недоступна для нажатия.

``` swift
case disabled(title: CardActionButtonTitle)
```

### `enabled`

Кнопка доступна для нажатия.

``` swift
case enabled(title: CardActionButtonTitle)
```
