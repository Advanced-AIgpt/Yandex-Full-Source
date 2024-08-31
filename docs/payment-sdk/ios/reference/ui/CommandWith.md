# CommandWith

Воспомогательный класс для хранения коллбека.

``` swift
public class CommandWith<T>: Equatable 
```

## Inheritance

`Equatable`

## Initializers

### `init(action:)`

``` swift
public init(action: @escaping (T) -> Void) 
```

## Operators

### `==`

``` swift
public static func == (lhs: CommandWith<T>, rhs: CommandWith<T>) -> Bool 
```
