# Hitchhiker's Guide to C++ in Arcadia: Concurrency and Parallelism

![](../img/dont_panic.png)

## Треды, процессы

## Future/Promise

Библиотека [library/cpp/threading/future](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future) реализует концепцию future/promise, позволяющую передавать результат (значение или исключение) асинхронной операции из одного контекста исполнения в другой.

Ближайшими аналогами [TFuture](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/future.h#L88) и [TPromise](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/future.h#L193) являются [std::shared_future](https://en.cppreference.com/w/cpp/thread/shared_future) и [std::promise](https://en.cppreference.com/w/cpp/thread/promise). Но, в отличие от них, `TFuture`/`TPromise` предоставляют больше возможностей. Наиболее полезными из которых являются continuations ([`Subscribe`](#future-subscribe) и [`Apply`](#future-apply)) и [примитивы ожидания](#wait-primitives) (`WaitAll`, `WaitAny`, etc). К сожалению, возможных [проблем](#future-problems) у наших реализаций тоже больше.

### Примеры

Простейший пример использования может выглядеть так:

```c++
#include <library/cpp/threading/future/future.h>

// Запускает асинхронную операцию
NThreading::TFuture<int> StartAsyncOperation() {
    auto promise = NewPromise<int>();
    RunOnASeparateThread(promise); // запускает операцию на отдельном потоке
    return promise.GetFuture();
}

// Поток 1 запускаем операцию
NThreading::TFuture<int> future = StartAsyncOperation();
...
// ждем завершения операции и получаем ее результат
auto value = future.GetValueSync();

// Поток 2 выполняет операцию
...
// устанавливаем результат
promise.SetValue(42);
```

Для получения результата может использоваться один из методов:

* `const T& GetValue(TDuration timeout)` - Ожидает установки значения или исключения (выставления) фьючи до истечения таймаута. Если фьюча была выставлена, то возвращает ссылку на константное значение фьючи или пробрасывает содержащееся в ней исключение. В противном случае выбрасывает исключение `TFutureException`. По умолчанию таймаут равен нулю, то есть, ожидание отсутствует.
* `const T& GetValueSync()` - Ожидает выставления фьючи сколь угодно долго. Возвращает ссылку на константное значение фьючи или пробрасывает содержащееся в ней исключение. Аналогичен `GetValue(TDuration::Max())`.
* `T ExtractValue(TDuration timeout)` - Ожидает выставления фьючи до истечения таймаута. Если фьюча была выставлена, то возвращает перемещенное значению фьючи или пробрасывает содержащееся в ней исключение. В противном случае выбрасывает исключение `TFutureException`. По умолчанию таймаут равен нулю, то есть, ожидание отсутствует. Если метод вернул значение, то последующие попытки получить значения фьючи (и всех фьюч, являющихся ее копиями) будут приводить к исключениям `TFutureException`.
* `T ExtractValueSync()` - Ожидает выставления фьючи сколь угодно долго. Возвращает перемещенное значение фьючи или пробрасывает содержащееся в ней исключение. Аналогичен `ExtractValue(TDuration::Max())`.

Для ожидания выставления значения (или исключения) можно использовать методы:

* `bool Wait(TDuration timeout)` - Ожидает выставления фьючи до истечения таймаута. Возвращает `true`, если значение было выставлено, и `false` в противном случае.
* `bool Wait(TInstant deadline)` - Ожидает выставления фьючи до заданного момента времени. Возвращает `true`, если значение было выставлено, и `false` в противном случае.
* `void Wait()` - Ожидает выставления фьючи сколь угодно долго. Аналогичен `Wait(TInstant::Max())`.

{#future-subscribe}

Также можно подписаться (установить continuation) на событие установки значения (исключения) с помощью метода `Subscribe`:

```c++
future.Subscribe([](const auto& f) {
    try {
        auto value = f.GetValue();
        ... // используем value
    } catch (const std::exception&) {
        ... // обрабатываем исключение, содержащееся в f
    } catch (...) {
        Y_FAIL(CurrentExceptionMessage());
    }
});
```

{% note warning %}

* Исключения не должны выходить за пределы обработчика
* Обработчик может быть вызван как асинхронно, так и синхронно в момент вызова `Subscribe`. Вызов будет синхронным, если future уже установлена в момент вызова Subscribe.
* Асинхронный вызов обработчика происходит в потоке, вызывающем `SetValue`/`SetException`. И, соответственно, он блокирует дальнейшее выполнение этого потока до своего завершения.

{% endnote %}

{#future-apply}

Если мы хотим не только выполнить какие-то действия после выставлению фьючи, но и вернуть наружу фьючу с результатом этих действий, удобно будет использовать метод `Apply`:

```c++
return future.Apply([future](const TFuture<TValue>&) mutable {
    // Здесь захват future выполняется исключительно ради того, чтобы можно было использовать метод ExtractValue. Он не является необходимым.
    auto value = future.ExtractValue();
    ... // что-то делаем
    return AnotherAsyncOperation(value)
            .Apply([](const TFuture<int>& f) {
                return f.GetValue() + 42;
            });
});
```

Результатом `Apply` всегда будет фьюча с "простым" типом значения. То есть, при возврате из обработчика фьючи произвольной вложенности `TFuture<TFuture<...<TFuture<T>>>>`, будет произведена развертка всех вложенных фьюч и результатом будет `TFuture<T>`. В приведенном примере наружу возвращается `TFuture<int>`. Данная особенность `Apply` позволяет запускать вложенные асинхронные операции без написания кода, пробрасывающего наружу их результаты.

Также этот пример демонстрирует возможность захвата и использования в обработчике фьючи, для которой вызывается `Apply`. Это позволяет, например, использовать метод `ExtactValue` вместо `GetValue`, избегая копирования значения фьючи. **Однако, этот прием создает циклическую ссылку фьючи на саму себя**. Данная циклическая ссылка уничтожается после установки значения фьючи и вызова обработчиков подписок. Но, если значение не будет выставлено, то фьюча и все ее подписки никогда не будут удалены (утечка памяти).

### Проблемы {#future-problems}

* Нет никаких гарантий, что фьюча когда либо будет выставлена. Если промис, соответствующий фьюче, будет разрушен до вызова `SetValue`/`SetException`, то фьюча никогда не будет выставлена. Это может приводить к неожиданным и неприятным эффектам. Например, к вечным ожиданиям или утечкам памяти. Данная проблема обсуждалась в рамках [IGNIETFERRO-1385](https://st.yandex-team.ru/IGNIETFERRO-1385).
* Нет никакой защиты от того, что подписка может кинуть исключение. В таком случае часть обработчиков может быть не вызвана, а исключение может полететь в поток, вызывающий `SetValue`/`SetException`.
* Асинхронный вызов обработчика подписки происходит в потоке, вызывающем `SetValue`/`SetException`. При этом у вызывающей `SetValue` стороны нет никаких средств для ограничения времени выполнения обработчика. То есть, медленный обработчик может сильно ухудшить производительность части приложения, которая отделена от него логически.
* Возможные скрытые копирования. Например, следующий код вызывает копирование объекта типа `T` при "раскрутке" вложенной фьючи:

```c++
TFuture<T> future = source.Apply([](const TFuture<T>& f) {
      return MakeFuture<T>();
});
```

* Отсутствие поддержки некопируемых (move-only) обработчиков подписок.

### Wait-примитивы {#wait-primitives}

Wait-примитивы создают фьючу, позволяющую ожидать выставления некоторого подмножества переданных на вход фьюч. В данный момент есть 3 примитива, для каждого из которых существует 2 реализации.

#### WaitAll

Представлен вариантами [NThreading::WaitAll](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/future.h) и [NThreading::NWait::WaitAll](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/subscription/wait_all.h). Первый вариант использует метод `Subscribe` фьюч напрямую. Второй вариант использует [менеджер подписок](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/subscription/README.md).
Первый вариант работает быстрее в сценариях, когда у передаваемых в `WaitAll` фьюч мало подписок (чаще всего это именно так). Второй вариант дополнительно предоставляет перегрузку для `std::initializer_list` и использует те же механизмы, что и остальные примитивы из пространства имен `NThreading::NWait`. Соответственно, первый вариант имеет смысл использовать там, где производительность операций подписки и вызова обработчиков может быть существенна на фоне времени ожидания выставления `WaitAll` и времени последующей обработки результатов фьюч, для которых создавался `WaitAll`. Второй вариант имеет смысл использовать там, где используются и другие примитивы из пространства имен `NThreading::NWait` (например, `WaitAny`).

**Примеры использования `WaitAll`**

Для пары фьюч:

```C++
#include <library/cpp/threading/future/future.h> // используем вариант NThreading::WaitAll

auto wait =  NThreading::WaitAll(future1, future2);
wait.Subscribe([future1 = std::move(future1), future2 = std::move(future2)](const TFuture<void>&) mutable {
    try {
        auto value1 = future1.ExtractValue();
        ... // обработка результата
    } catch (...) {
        Y_FAIL(CurrentException());
    }
    try {
        auto value2 = future2.ExtractValue();
        ... // обработка результата
    } catch (...) {
        Y_FAIL(CurrentException());
    }
});
```

Для множества фьюч:

```C++
#include <library/cpp/threading/future/subscription/wait_all.h> // используем вариант NThreading::NWait::WaitAll

TVector<TFuture<void>> futures = ...;
...
NThreading::NWait::WaitAll(futures).Wait();
// обработка результатов
for (auto& f : futures) {
    try {
        auto value = f.ExtractValue();
        ... // обработка результата
    } catch (const std::exception&) {
        ... // обработка исключения
    }
}
```

#### WaitAny {#waitany}

Представлен вариантами [NThreading::WaitAny](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/future.h) и [NThreading::NWait::WaitAny](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/subscription/wait_any.h). Так же, как и `WaitAll`, первый вариант использует метод `Subscribe` фьюч напрямую. Второй вариант использует [менеджер подписок](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/subscription/README.md).
Первый вариант может быстрее в сценариях, когда у передаваемых в `WaitAny` фьюч мало подписок. Однако, у него есть существенный недостаток:

{% note warning %}

Ко всем переданным в `WaitAny` фьючам добавляются подписки. И эти подписки не удаляются после того, как будет выставлена фьюча, порожденная `WaitAny` (поскольку `TFuture` просто не имеет механизма для удаления подписок). Соответственно, если мы будем неоднократно передавать в вызовы `WaitAny` какое-то множество фьюч, остающихся невыставленными, их списки подписок будут расти (и, заодно, не будут удаляться прокси-объекты, создаваемые вызовами `WaitAny`). То есть, будет расти потребление памяти, и последующие вызовы `SetValue/SetException` для этих фьюч будут работать медленнее.

{% endnote %}

Так что, прежде чем использовать первый вериант `WaitAny`, стоит оценить возможные риски от его использования. В сценариях, где одно и то же множество фьюч неоднократно используется в вызовах `WaitAny`, вероятно, стоит предпочесть второй вариант (`NThreading::NWait::WaitAny`), который корректно удаляет подписки после выставления порожденной им фьючи.

В качестве примера фьючи, с которой точно не стоит использовать первый вариант, можно привести фьючу, получаемую из [CancellationToken](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/cancellation/README.md). Такая фьюча, как правило, используется в множестве вызовов `WaitAny` (как признак отмены операции) и может оставаться невыставленной на протяжении длительного времени.

**Примеры использования `WaitAny`**

Для пары фьюч:

```C++
#include <library/cpp/threading/future/future.h> // используем вариант NThreading::WaitAny

NThreading::WaitAny(future1, future2).Wait();
// Мы не знаем, какая из фьюч была выставлена (возможно, что обе), поэтому проверяем все
for (auto& f : {future1, future2}) {
    if (f.HasValue()) {
        auto value = f.ExtractValue();
        ... // обработка результата
    } else if (f.HasException()) {
        try {
            f.TryRethrow();
        } catch (const std::exception&) {
            ... // обработка исключения
        }
    }
}
```

Для множества фьюч:

```C++
#include <library/cpp/threading/future/subscription/wait_any.h> // используем вариант NThreading::NWait::WaitAny

TVector<TFuture<void>> futures = ...;
auto wait = NThreading::NWait::WaitAny(futures);
return wait.Apply([futures = std::move(futures)](const TFuture<void>&) mutable {
    for (auto& f : futures) {
        if (f.HasValue()) {
            auto value = f.ExtractValue();
            ... // обработка результата
        } else if (f.HasException()) {
            try {
                f.TryRethrow();
            } catch (const std::exception&) {
                ... // обработка исключения
            }
        }
    }
});
```

#### WaitAllOrException

Данный примитив ведет себя как `WaitAll` при отстутствии исключений, и как `WaitAny`, когда любая из фьюч выставляется исключением.
Представлен вариантами [NThreading::WaitExceptionOrAll](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/future.h) и [NThreading::NWait::WaitAllOrException](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/threading/future/subscription/wait_all_or_exception.h). Различия вариантов аналогичны различиям для [WaitAny](#waitany), но возникновение проблемы разрастания списков подписок менее вероятно, поскольку разрастание возможно только при выставлении порожденной фьючи по исключению.

**Примеры использования `WaitAllOrException`**

Пример для пары фьюч:

```C++
#include <library/cpp/threading/future/future.h> // используем вариант NThreading:::WaitExceptionOrAll

auto wait = NThreading::WaitExceptionOrAll(future1, future2);
wait.Wait();
if (wait.HasValue()) {
    auto value1 = future1.ExtractValue();
    auto value2 = future2.ExtractValue();
    ... // используем значения
} else {
    Y_VERIFY(wait.HasException());
    .. // тут обработка полностью аналогична WaitAny
}
```

Пример для множества фьюч:

```C++
#include <library/cpp/threading/future/subscription/wait_all_or_exception.h> // используем вариант NThreading::NWait::WaitAllOrException

TVector<TFuture<void>> futures = ...;
auto wait = NThreading::NWait::WaitAllOrException(futures);
return wait.Apply(futures = std::move(futures)](const TFuture<void>& f) mutable {
    if (f.HasValue()) {
        // все фьючи имеют валидные значения
        for (auto& fut : futures) {
            auto value = fut.ExtractValue();
            ... // используем значение
        }
    } else {
        Y_VERIFY(f.HasException());
        ... // тут обработка полностью аналогична WaitAny
    }
});
```
