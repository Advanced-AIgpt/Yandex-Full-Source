# CustomCellProvider

Протокол провайдера кастомных ячеек в таблицу методов.

``` swift
public protocol CustomCellProvider 
```

## Requirements

### customCellView(\_:​from:​)

Вернуть ячейку для отображения соответствующих props.

``` swift
func customCellView(_ tableView: UITableView, from cell: SelectCellProps) -> UITableViewCell
```

#### Parameters

  - tableView: исходная таблица.
  - cell: данные для ячейки.

#### Returns

вью ячейки.

### customCellHeight(\_:​for:​isSelected:​)

Вернуть высоту кастомной ячейки.

``` swift
func customCellHeight(_ tableView: UITableView, for cell: SelectCellProps, isSelected: Bool) -> CGFloat
```

#### Parameters

  - tableView: исходная таблица.
  - cell: данные для ячейки.
  - isSelected: выбрана ли ячейка.

#### Returns

высота в `CGFloat`.
