# C++ : модули

Модули, описывающие C++-сборку
- [LIBRARY](#library)
- [PROGRAM](#program)
- [DLL](#dll)
- [DLL_FOR](#dll_for)
- [DLL_TOOL](#dll_tool)
- [SO_PROGRAM](#so_program)
- [FAT_OBJECT](#fat_object)
- [UNITTEST](#unittest)
- [UNITTEST_FOR](#unittest_for)
- [UNITTEST_WITH_CUSTOM_ENTRY_POINT](#unittest_with_custom_entry_point)
- [GTEST](#gtest)
- [G_BENCHMARK](#g_benchmark)
- [Y_BENCHMARK](#y_benchmark)
- [FUZZ](#fuzz)


## LIBRARY

`LIBRARY()`

Модуль для создания статической библиотеки.

Исходные файлы, участвующие в сборке библиотеки, должны быть перечислены в макросах `SRCS`. Кроме `*.cpp` файлов в `SRCS` макросах могут быть указаны файлы с другими расширениями, например, `*.proto`, `*.swg`, `*.fbs` и другие. Кроме того, можно использовать макросы кодогенерации такие, как `RUN_PROGRAM`, `GENERATE_ENUM_SERIALIZATION`.

Этот модуль является промежуточным, поэтому при непосредственной сборке его зависимости пересобираться не будут. Все зависимые по `PEERDIR` библиотеки транзитивно попадают в конечную цель - программу или динамическую библиотеку, где все `LIBRARY` модули собираются и линкуются вместе

Так как это C++ библиотека, то при переходе по `PEERDIR` в мультимодуль выбирается соответствующий `CPP` модуль.

Не имеет смыслу указывать `LIBRARY` в `DEPENDS` или `BUNDLE`, так как она не является автономной сущностью.

Для того чтобы использовать библиотеку в тестах, нужно указать на нее `PEERDIR` из тестов.

**Пример:**
```
OWNER(g:my_group)
LIBRARY()
    PEERDIR(
        library/cpp/http/client
        contrib/libs/fmt
    )

    SRCS(
        x.cpp
        y.proto
        z.pyx
    )
END()
```

{% note info %}

Если вы считаете, что вам нужно распространить статическую библиотеку, пожалуйста, свяжитесь с devtools@ для получения помощи.

{% endnote %}


## PROGRAM

`PROGRAM([progname])`

Программа на C++. Если указан `progname`, то имя программы будет `progname`, иначе имя будет сгенерировано из имени директории, в которой определен `PROGRAM`.

- Собирается в исполняемый файл под целевую платформу.

- Статически линкует все библиотеки, от которых зависит по `PEERDIR`.

- Статически линкует все глобальные объектники, которые перечислены в статических библиотеках через макрос `GLOBAL_SRCS` или которые помечены `GLOBAL` в макросе `SRCS`.

**Пример:**
```
PROGRAM(example)
OWNER(g:my-team-group)

SRCS(main.cpp)
PEERDIR(arcadia/top_level_dir/my_lib)

END()
```


## DLL

`DLL(name major_ver [minor_ver] [PREFIX prefix])`

Модуль для создания динамической библиотеки.

1. `major_ver` и `minor_ver` должны быть целыми числами.
2. `PREFIX` позволяет изменить префикс названия библиотеки (по умолчанию у `DLL` префикс `lib`).

Поведение схоже с поведением модуля `PROGRAM`. Так же, как и последний, он статически линкуется со всеми модулями `LIBRARY`, от которых зависит, требуя для своей сборки версию статической библиотеки с поддержкой Position Independent Code.

Для модуля `DLL` опционально можно указывать export script со списком символов, которые должны быть доступны из динамической библиотеки. Для этого используется макрос `EXPORTS_SCRIPT`.

`DLL` не участвует в линковке программы. Она может быть использована из Java или как финальный артефакт.

**Пример:**
```
DLL()
OWNER(g:my-team-group)

SRCS(
    some_src.cpp
    other_src.cpp
)
EXPORTS_SCRIPT(myproj.exports)

END()
```

## DLL_FOR

`DLL_FOR(path/to/lib [libname] [major_ver [minor_ver]] [EXPORTS symlist_file])`

Модуль, собирающий модуль `LIBRARY()` как динамическую библиотеку. 

1. `path/to/lib` путь до библиотеки.
2. `libname` имя DLL библиотеки.
3. `major_ver` и `minor_ver` должны быть целыми числами.
4. `EXPORTS` позволяет указывать export script со списком символов. которые должны быть доступны из динамической библиотеки.

**Пример структуры директорий**
```
arcadia/top_level_dir/my_lib/
                      |-> dll/
                      |   |-> my_dll.exports
                      |   |-> ya.make
                      |-> some_src.cpp
                      |-> other_src.cpp
                      |-> ya.make
```

**Пример ya.make файла**
```
DLL_FOR(arcadia/top_level_dir/my_lib/ my_dll EXPORTS my_dll.exports)
```


## DLL_TOOL


## SO_PROGRAM


## FAT_OBJECT


## UNITTEST

`UNITTEST([name])`

Модуль для тестирования, использующий внутреннюю библиотеку `library/cpp/testing/unittest`.

Рекомендуется не указывать `name` в этом модуле.

Подробная документация указана здесь: [unittest](../tests/cpp.md#unittest)

**Пример:**
```
OWNER(rb:yatool)

UNITTEST()

SRCS(
    test.cpp
)

END()
```


## UNITTEST_FOR


## GTEST

`GTEST([name])`

Модуль для тестирования, использующий библиотеку `library/cpp/testing/gtest`.

Рекомендуется не указывать `name` в этом модуле.

Подробная документация указана здесь: [gtest](../tests/cpp.md#gtest)

**Пример:**
```
GTEST()

OWNER(rb:yatool)

SRCS(test.cpp)

END()
```


## G_BENCHMARK

`G_BENCHMARK([benchmarkname])`

Модуль для запуска бенчмарков, использующий [google benchmark](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/benchmark/README.md)

Подробная документация указана здесь: [cpp_benchmark](../tests/benchmark)


## Y_BENCHMARK

`Y_BENCHMARK([benchmarkname])`

{% note alert %}

DEPRECATED.

{% endnote %}

Модуль для запуска бенчмарков, использующий `library/cpp/testing/benchmark`.


## FUZZ
