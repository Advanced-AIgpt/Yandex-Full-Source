# SelectMethodTableView

Таблица для отображения списка методов оплат. Может быть расширена и отображать кастомные данные вместе с методами.

``` swift
public final class SelectMethodTableView: UITableView 
```

## Inheritance

`UITableView`, `UITableViewDataSource`, `UITableViewDelegate`

## Properties

### `customCellProvider`

Провайдер кастомных ячеек.

``` swift
public var customCellProvider: CustomCellProvider?
```

## Methods

### `animateHeightChange()`

Анимировать изменения.

``` swift
public func animateHeightChange() 
```

### `calculateViewHeight()`

Посчитать требуемую высоту для отображения всего контента в таблице.

``` swift
public func calculateViewHeight() -> CGFloat 
```

#### Returns

высота в `CGFloat`

### `focusCvnInputField()`

Поставить фокус на ввод CVC/CVV в выбранной ячейке, если это возможно

``` swift
public func focusCvnInputField() 
```

### `setProps(_:)`

Задать данные для таблицы.

``` swift
public func setProps(_ props: SelectMethodProps) 
```

#### Parameters

  - props: данные.

### `tableView(_:numberOfRowsInSection:)`

``` swift
final public func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int 
```

### `tableView(_:viewForHeaderInSection:)`

``` swift
final public func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? 
```

### `tableView(_:viewForFooterInSection:)`

``` swift
final public func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? 
```

### `tableView(_:heightForHeaderInSection:)`

``` swift
final public func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat 
```

### `tableView(_:heightForFooterInSection:)`

``` swift
final public func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat 
```

### `tableView(_:cellForRowAt:)`

``` swift
final public func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell 
```

### `tableView(_:didSelectRowAt:)`

``` swift
final public func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) 
```

### `tableView(_:heightForRowAt:)`

``` swift
final public func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat 
```

### `scrollViewDidScroll(_:)`

``` swift
final public func scrollViewDidScroll(_ scrollView: UIScrollView) 
```
