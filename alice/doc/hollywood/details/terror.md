# Класс TError

Класс для возврата информации об ошибке в процессе обработки сценария.

Диспетчер, функции сцен и рендеры могут воспользоваться этим классом или макросом `HW_ERROR()` для прерывания работы сценария и сигнала о некорректной работе.

Доступно 2 варианта возврата данных об ошибке:

1. Сконструировать экземпляр класса `TError`, добавить при необходимости дополнительную диагностику и вернуть экземпляр этого класса во фреймворк.
2. Вызвать макрос `HW_ERROR()`, который бросит исключение, перехваченное в корне фреймворка.

Примеры:

```c++
// Вариант 1.
TRetScene TRandomNumberScenario::Dispatch(const TRunRequest& runRequest, const TStorage& storage, const TSource& source) const {
    // ...

    TError err(TError::EErrorDefinition::SubsystemError);
    err.Details() << "Some details about " << "this error";
    return err;
}

// Вариант 2.
TRetScene TRandomNumberScenario::Dispatch(const TRunRequest& runRequest, const TStorage& storage, const TSource& source) const {
    // ...

    HW_ERROR("Some details about " << "this error");
    // Макрос бросит исключение, сюда мы уже не придем.
}
```