# Go : модули

Модули описывающие Go-сборку
- [GO_LIBRARY](#go_library)
- [GO_PROGRAM](#go_program)
- [GO_TEST](#go_test)
- [GO_TEST_FOR](#go_test_for)
- [GO_DLL](#go_dll)

## GO_LIBRARY
`GO_LIBRARY()`

Этот модуль собирает `GO` пакет. Исходные файлы, участвующие в сборке пакета, должны быть явно перечислены в макросах [SRCS](macros.md#srcs) или [CGO_SRCS](macros.md#cgo_srcs). В макросе [SRCS](macros.md#srcs) перечисляются исходные файлы на `GO`, `C`/`C++`, `Asm`, а в макросе [CGO_SRCS](macros.md#cgo_srcs) перечисляются `CGO` файлы. Кроме того, можно использовать макросы кодогенерации такие как `RUN_PROGRAM`, `PYTHON` etc. Результатам кодогенрации автоматически применяется макрос [SRCS](macros.md#srcs) (если результаты макросов кодогенерации перечислены в OUT параметрах). Исходные файлы тестов пакета перечисляются в макросах [GO_TEST_SRCS](macros.md#go_test_srcs) и [GO_XTEST_SRCS](macros.md#go_xtest_srcs). В макросе [GO_TEST_SRCS](macros.md#go_test_srcs) перечисляются исходные файлы для "внутренних" тестов, а в макросе [GO_XTEST_SRCS](macros.md#go_xtest_srcs) - для "внешних" тестов. `PEERDIR` на `PROTO_LIBRARY` из этого модуля выберет `GO` версию подмодуля `PROTO_LIBRARY`.

Пример:
```
OWNER(g:my_group)

GO_LIBRARY()
    SRCS(
        file1.go
        file2.go
        file3.c
    )

    CGO_SRCS(
        cgo.go
    )

    PEERDIR(
        my/lib/go
        my/lib/c
    )
END()
```

## GO_PROGRAM
`GO_PRGRAM()`

Этот модуль собирает программу на `GO`. Всё сказанное про макросы [SRCS](macros.md#srcs)/[CGO_SRCS](macros.md#cgo_srcs)/[GO_TEST_SRCS](macros.md#go_test_srcs)/[GO_XTEST_SRCS](macros.md#go_xtest_srcs) для модуля [GO_LIBRARY](#go_library) справедливо и для модуля `GO_PROGRAM`.
`PEERDIR` на `PROTO_LIBRARY` из этого модуля выберет GO версию подмодуля `PROTO_LIBRARY`.

Пример:
```
OWNER(g:my_group)

GO_PROGRAM()

SRCS(
    file1.go
    file2.go
)

END()
```
## GO_TEST
`GO_TEST()`

Этот модуль собирает тесты на `GO`. Всё сказанное про макросы [SRCS](macros.md#srcs)/[CGO_SRCS](macros.md#cgo_srcs)/[GO_TEST_SRCS](macros.md#go_test_srcs)/[GO_XTEST_SRCS](macros.md#go_xtest_srcs) для модуля [GO_LIBRARY](#go_library) справедливо и для модуля `GO_TEST`. Обычно этот модуль используется для написания интеграционных тестов (см. [GO_TEST_FOR](#go_test_for) для сборки тестов для `GO` пакета/программы)

Пример:
```
OWNER(g:my_group)

GO_TEST()

GO_XTEST_SRCS(
    acceptance_test.go
)

SIZE(LARGE)

TAG(ya:fat)

END()
```

## GO_TEST_FOR
`GO_TEST_FOR(<package-dir>)`

Этот модуль собирает тесты для `GO` пакета/программы, определяемым параметром `<package-dir>` (путь до директории пакета/программы). Этот модуль поддерживает "стандартные" тестовые макросы, такие как `DATA`, `DEPENDS`, `SIZE`, `REQUIREMENTS`, `TAG` etc.

{% note warning %}

В этом модуле строго не рекомендовано использовать [GO_TEST_SRCS](macros.md#go_test_srcs) и [GO_XTEST_SRCS](macros.md#go_xtest_srcs) - эти макросы должны быть определены в модуле `GO` пакета/программы из `<package-dir>`.

{% endnote %}

Пример:
```
OWNER(g:my_group)

GO_TEST_FOR(my/lib/go)

SIZE(MEDIUM)

END()
```

## GO_DLL
`GO_DLL([name] [major_ver] [minor_ver] [PREFIX prefix])`

Этот модуль собирает динамическую библиотеку. Всё сказанное про макросы [SRCS](macros.md#srcs)/[CGO_SRCS](macros.md#cgo_srcs)/[GO_TEST_SRCS](macros.md#go_test_srcs)/[GO_XTEST_SRCS](macros.md#go_xtest_srcs) для модуля [GO_LIBRARY](#go_library) справедливо и для модуля `GO_DLL`.

```
OWNER(g:my_group)

GO_DLL(mylib 2)

GO_LINK_FLAGS(-s -w)

SRCS(
    file1.go
    file2.go
)

CGO_SRCS(
    cgo.go
)

END()
```
