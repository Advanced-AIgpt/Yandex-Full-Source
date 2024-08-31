# Extensions on Swift.Result

## Methods

### `onValue(_:)`

``` swift
@discardableResult
    func onValue(_ f: (Success) throws -> Void) rethrows -> Swift.Result<Success, Failure> 
```

### `onError(_:)`

``` swift
@discardableResult
    func onError(_ f: (Failure) -> Void) -> Swift.Result<Success, Failure> 
```

### `toOptional()`

``` swift
func toOptional() -> Success? 
```

### `toError()`

``` swift
func toError() -> Failure? 
```
