# SelectMethodProps

Свойства таблицы.

``` swift
public struct SelectMethodProps 
```

## Nested Type Aliases

### `SelectedIndex`

Алиас индексового типа.

``` swift
public typealias SelectedIndex = Int
```

## Initializers

### `init(selectedMethodIndex:focusFirstCVV:showRadioButtons:enableScroll:cells:onCvnInputReady:onMethodSelect:)`

``` swift
public init (selectedMethodIndex: SelectedIndex?,
                 focusFirstCVV: Bool,
                 showRadioButtons: Bool,
                 enableScroll: Bool,
                 cells: [SelectCellProps],
                 onCvnInputReady: CommandWith<CvnInput?>,
                 onMethodSelect: CommandWith<SelectedIndex>) 
```
