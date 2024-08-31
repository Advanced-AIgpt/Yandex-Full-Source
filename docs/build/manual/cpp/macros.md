# С++ : макросы

(частично) [https://wiki.yandex-team.ru/devtools/commandsandvars/](https://wiki.yandex-team.ru/devtools/commandsandvars/)

Макросы для сборки C++

{% note info %}

Work in progress

```
NO_LIBC() - need description
NO_PLATFORM() - need description

NOLTO()/NOSSE()/NO_SSE4()/...

SRC() c флагами

STRIP()/SPLIT_DWARF()

BASE_CODEGEN()/STRUCT_CODEGEN()/SPLIT_CODEGEN()

```

{% endnote %}

## CFLAGS

`CFLAGS([GLOBAL compiler_flag]* compiler_flags)`

Добавляет указанные флаги в команду компиляции C/C++ файлов.

Параметр `GLOBAL` распространяет указанные флаги на зависимые проекты.

{% note info %}

Помните о несовместимости флагов для clang и cl. Чтобы задать флаги для cl используйте `MSVC_FLAGS`.

{% endnote %}


## CONLYFLAGS

`CONLYFLAGS([GLOBAL compiler_flag]* compiler_flags)`

Добавляет указанные флаги в команду компиляции C файлов (`*.c`).

Параметр `GLOBAL` распространяет указанные флаги на зависимые проекты.


## CXXFLAGS

`CXXFLAGS(compiler_flags)`

Добавляет указанные флаги в команду компиляции C++ файлов (`*.cpp`).

Параметр `GLOBAL` распространяет указанные флаги на зависимые проекты.


## MSVC_FLAGS

`MSVC_FLAGS([GLOBAL compiler_flag]* compiler_flags)`

Добавляет указанные флаги в команду компиляции C/C++ файлов.

Параметр `GLOBAL` распространяет указанные флаги на зависимые проекты.

Флаги применяются только, если используется компилятор MSVC (cl.exe).


## LDFLAGS

`LDFLAGS(LinkerFlags...)`

Добавляет указанные флаги в команду линковки программы или динамической библиотеки.

{% note info %}

Эти флаги всегда являются глобальными. При добавлении их в LIBRARY модуль, они будут влиять на все программы, динамические библиотеки и тесты, с которыми линкуется эта библиотека.

{% endnote %}

{% note info %}

Помните о несовместимости флагов для gcc и link.

{% endnote %}


## EXTRALIBS

`EXTRALIBS(Libs...)`

Добавляет указанные внешние *динамические* библиотеки в линковку программы.


## EXTRALIBS_STATIC

`EXTRALIBS_STATIC(Libs...)`

Добавляет указанные внешние *статические* библиотеки в линковку программы.


## WERROR

`WERROR()`

Считает warning как error в текущем модуле.

В светлом будущем будет удален, так как `WERROR` будет включен по умолчанию.

Приоритеты: `NO_COMPILER_WARNINGS` > `NO_WERROR` > `WERROR_MODE` > `WERROR`.


## COMPILE_C_AS_CXX


## NO_WERROR

`NO_WERROR()`

Обратное поведение к `WERROR`.

Приоритеты: `NO_COMPILER_WARNINGS` > `NO_WERROR` > `WERROR_MODE` > `WERROR`.


## NO_COMPILER_WARNINGS

`NO_COMPILER_WARNINGS()`

Отключает все предупреждения компилятора в текущем модуле.

Приоритеты: `NO_COMPILER_WARNINGS` > `NO_WERROR` > `WERROR_MODE` > `WERROR`.


## NO_OPTIMIZE

`NO_OPTIMIZE()`

Собирает код без всех оптимизаций (-O0 mode).


## NO_UTIL

`NO_UTIL()`

Собирает модуль без зависимости на [`util`](https://a.yandex-team.ru/arc/trunk/arcadia/util).


## NO_RUNTIME

`NO_RUNTIME()`

Этот макрос вызывает макрос `NO_UTIL` и отключает переменную `USE_ARCADIA_LIBM`.

Если в модуле, содержащем `NO_RUNTIME`, есть `PEERDIR` на модуль, не содержащий `NO_RUNTIME`, то будет предупреждение.


## NO_LIBC

`NO_LIBC()`


## NO_PLATFORM

`NO_PLATFORM()`


## NO_DEBUG_INFO


## NO_SSE4


## NO_WSHADOW


## STRIP


## SPLIT_DWARF


## ALLOCATOR

`ALLOCATOR(Alloc)`

Устанавливает аллокатор Alloc для программ и динамических библиотек. По умолчанию используется `LF`.

{% note warning %}

Использование не в модулях программ и динамических библиотек приводит к ошибкам конфигурации.

{% endnote %}

Доступны следующие реализации аллокаторов:

- LF - [lfalloc](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/lfalloc)
- LF_YT - [lfalloc for yt](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/lfalloc/yt/ya.make)
- LF_DBG -  [debug allocator](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/lfalloc/dbg/ya.make)
- YT - [YT allocator](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/ytalloc/impl/ya.make)
- J - [JEMalloc allocator](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/malloc/jemalloc)
- B - [balloc allocator](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/balloc)
- BM - balloc for market
- C - Похож на `B`, но может быть переключен для каждого потока в `LF` или `SYSTEM`
- TCMALLOC -  [Google TCMalloc](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/malloc/tcmalloc)
- GOOGLE -  [Google TCMalloc ](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/malloc/galloc)
- LOCKLESS - [Allocator based upon lockless queues](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/malloc/lockless)
- SYSTEM - Использовать аллокатор системы
- FAKE - Не использовать аллокатор

Более подробно об аллокаторах: [allocators](https://wiki.yandex-team.ru/arcadia/allocators/)


## GENERATE_ENUM_SERIALIZATION

`GENERATE_ENUM_SERIALIZATION(File.h)`

Строит функции ввода-вывода членов перечислений, описанных в заголовке, и компилирует их в модуль.

Сериализатор понимает перечисления в пространствах имён и структурах (классах), а также поддерживает значения, задаваемые вручную. `enum class` тоже поддерживаются.

**Пример:**

*ya.make* файл:
```
LIBRARY()

SRCS(src.cpp)

GENERATE_ENUM_SERIALIZATION(unit.h)

END()
```

*unit.h*

```
struct TDesc {
    enum EKind {
        K_SOURCE = 2 /* "source" */,
        K_HEADER = 3 /* "header" */,
        K_ASM    /* "asm" */
    };
};
```

Тогда в *src.cpp* можно написать так:

```
#include "unit.h"
#include <util/stream/ios.h>
// ...
void f() {
    TDesc::EKind kind = TDesc::K_ASM;
    Cout << "kind is " << kind << Endl;
}
```

При использовании этого макроса для перечислений поддерживаются следующие функции из [util/string/cast.h](https://a.yandex-team.ru/arc/trunk/arcadia/util/string/cast.h):

```
template <>
void Out<TDesc::EKind>(TOutputStream& os, TDesc::EKind& n);

template <>
bool TryFromString<TDesc::EKind>(...)

template <>
TDesc::EKind FromString<TDesc::EKind>(...)

const TString& ToString(TDesc::EKind value);                    // переводит значение enum-а в строку
bool TryFromString(const TStringBuf& name, TDesc::EKind& ret);  // переводит строку в значение enum-а (если не найдено, возвращает false)
```

и следующие функции из [util/generic/serialized_enum.h](https://a.yandex-team.ru/arc/trunk/arcadia/util/generic/serialized_enum.h):

```
template<>
const TString& GetEnumAllNames<TDesc::EKind>();                // возвращает полный список значений enum-а через запятую: 'source', 'header', 'asm'

template<>
const TVector<TDesc::EKind>& GetEnumAllValues<TDesc::EKind>(); // возвращает список значений enum-а
```

{% note warning %}

Сериализация генерируется для потоков `Out` из `util/stream`, а не для стандартных потоков (`std::ostream`).
В Аркадии [рекомендуется использовать потоки из `util`](https://docs.yandex-team.ru/arcadia-cpp/cpp_library_guide).
Реализация потоков в `util` значительно легче и эффективнее стандартной.

{% endnote %}


## GENERATE_ENUM_SERIALIZATION_WITH_HEADER

`GENERATE_ENUM_SERIALIZATION_WITH_HEADER(File.h)`

Используется вместо макроса [`GENERATE_ENUM_SERIALIZATION`](#generate_enum_serialization). Дополнительно генерирует `.h` файл с именем `*.h_serialized.h` с интерфейсом, объявленным в [util/generic/serialized_enum.h](https://a.yandex-team.ru/arc/trunk/arcadia/util/generic/serialized_enum.h).


## JOIN_SRCS

`JOIN_SRCS(Out Src...)`

Объединяет множество исходных файлов в один файл с именем `Out` и передает его для дальнейшей обработки. Этот макрос не копирует все содержимое `Src` файлов в `Out` файл, а только помещает `#include "Src"` в `Out` файл.

Используйте только для C++ файлов.

Важно указать расширение Out файла, так как от этого зависит его дальнейшая обработка.

**Пример:**

```
PROGRAM()

JOIN_SRCS(
    all_srcs.cpp
    first.cpp
    second.cpp
    third.cpp
)

END()
```

В итоге файл `all_srcs.cpp` будет выглядеть так:

```
...
#include "first.cpp"
#include "second.cpp"
#include "third.cpp"
```


## JOIN_SRCS_GLOBAL


## EXPORTS_SCRIPT


## SUPPRESSIONS
