# Класс TReturnValue

Классы `TReturnValueXxx` определяет собой группу классов, которые используются как основной код возврата из функций диспетчера и сцен для последующей передачи управления:

* Из диспетчера — в выбранную сцену или иррелевантный рендер.
* Из сцены — в выбранный рендер.
* Из бизнес-логики - в ручки `Continue`/`Apply`/`Commit`.

Классы `TReturnValueXxx` обычно формируется прямо в момент `return` и содержат от 0 до 3 полей.

## Выбор сцены

Класс `TReturnValue` вызывается из `Dispatch()` для выбора сцены.

* Прототип конструктора для выбора сцены с сетевым походом:
    ```cpp
    template <class Object, class T, typename Ret>
    TReturnValueScene(Ret (Object::*)(const T&, const TRunRequest&, const TStorage&) const, const T& proto, const TString& selectedSf = "");
    ```

* Прототип конструктора для выбора сцены без сетевого похода:
    ```cpp
    template <class Object, class T, typename Ret>
    TReturnValueScene(Ret (Object::*)(const T&, const TRunRequest&, TStorage&, const TSource&) const, const T& proto, const TString& selectedSf = "");
    ```

Аргументы конструктора:

* адрес функции-обработчика сцены;
* указатель на протобаф с аргументами (которые передаются в обработчик первым параметром);
* (опционально) указатель на выбранный семантический фрейм.


### Пример

Сцена `TMyScene` принимает протобаф `Message TMyProto`. Класс описания сцены:

```
class TMyScene : public TScene {
    ...
    TRetRender Main(const TMyProto&,
                    const TRunRequest&,
                    TStorage&,
                    const TSource&) const;
}
```

Для выбора указанной сцены в диспетчере напишите следующий код:

```cpp
TMyProto proto;
... // заполните proto данными, которые будет переданы в сцену
return TReturnValueScene(&TMyScene::Main, proto, selectedSemanticFrameName);
```

Передача фреймворку названия выбранного семантического фрейма позволит:

* автоматически занести этот SF в ответ мегамайнду;
* настроит `SetIntentName()` для блока аналитики (вы можете переопределить название интента позднее.

## Выбор рендера

Класс `TReturnValueRender` вызывается из `Scene::Main()` для выбора рендера.

* Прототип конструктора, если рендер является функцией-членом класса `TScenario`:
    ```cpp
    template <class Object, class T, typename Ret>
    TReturnValueRender(Ret (Object::*)(const T&, const TRender& render) const, const T& proto);
    ```
* Прототип конструктора, если рендер является статической или свободной функцией:
    ```cpp
    template <class T, typename Ret>
    TReturnValueRender(Ret (*)(const T&, const TRender& render), const T& proto);
    ```

### Пример

Рендер как свободная функция:
```cpp
extern TRetResponse MyRenderScene(const TMyRenderProto& args, const TRender& render);
```

Для выбора указанной сцены в диспетчере напишите следующий код:

```cpp
TMyRenderProto proto;
... // заполните proto данными, которые будет переданы в рендерер
return TReturnValueRender(&MyRenderScene, proto);
```

## Генерация иррелевантного ответа

Класс `TReturnValueRenderIrrelevant` полностью аналогичен `TReturnValueRender()`, но позволяет сгенерировать иррелевантный ответ. Вызывается из `Scenario::Dispatch()` или из `Scene::Main()`.


## Миграция старых сценариев

Класс `TReturnValueDo` используется только в процессе миграции старых сценариев на Hollywood Framework. Возвращается из функции `DispatchSetup()` или из `Dispatch()` и позволяет переключиться на старый флоу выполнения сценария. 

Чтобы воспользоваться этим функционалом, в Hollywood должно быть определено 2 сценария с **одинаковым именем**:
* первый сценарий реализован традиционным способом;
* второй — через Hollywood Framework.


Пример возврата из диспетчера сценария:

```cpp
    return TReturnValueDo();
```

Подробная информация о `TReturnValueDo` в разделе [Перенос действующих сценариев](../compatibility/migration.md).
