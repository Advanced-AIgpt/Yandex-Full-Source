# Go : макросы

Макросы для сборки GO:
- [SRCS](#srcs)
- [CGO_SRCS](#cgo_srcs)
- [GO_TEST_SRCS](#go_test_srcs)
- [GO_XTEST_SRCS](#go_xtest_srcs)
- [CGO_CFLAGS](#cgo_cflags)
- [CGO_LDFLAGS](#cgo_ldflags)
- [GO_ASM_FLAGS](#go_asm_flags)
- [GO_COMPILE_FLAGS](#go_compile_flags)
- [GO_LINK_FLAGS](#go_link_flags)
- [GO_PACKAGE_NAME](#go_package_name)
- [GO_SKIP_TESTS](#go_skip_tests)
- [GO_PROTO_PLUGIN](#go_proto_plugin)
- [GO_GRPC_GATEWAY_SRCS](#go_grpc_gateway_srcs)
- [GO_GRPC_GATEWAY_SWAGGER_SRCS](#go_grpc_gateway_swagger_srcs)
- [GO_MOCKGEN_FROM](#go_mockgen_from)
- [GO_MOCKGEN_TYPES](#go_mockgen_types)
- [GO_MOCKGEN_REFLECT](#go_mockgen_reflect)
- [GO_MOCKGEN_MOCKS](#go_mockgen_mocks)
- [USE_CXX](#use_cxx)
- [USE_UTIL](#use_util)

## SRCS
В макросе `SRCS` должны быть явно перечислены все исходные файлы (на языках `GO`, `Asm`, `C/C++`), необходимые для сборки пакета. Исходные файлы, которые являются являются результатом генерации таких макросов, как `RUN_PROGRAM`, `PYTHON` etc и перечислены в параметрах с ключевым словом `OUT`, добавляются в сборку пакета автоматически.

{% note warning %}

Файлы, относящиеся к `CGO`, должны быть перечислены в макросе [CGO_SRCS](#cgo_srcs).

{% endnote %}

## CGO_SRCS
В макросе `CGO_SRCS` должны быть явно перечислены все исходные файлы на языке `GO`, относящиеся к `CGO`. Если `CGO` файл является результатом генерации таких макросов как `RUN_PROGRAM`, `PYTHON` etc, то такие файлы должны быть перечислены в параметрах этих макросов с ключевым словом `OUT_NOAUTO` и явно указаны в вызове макроса `CGO_SRCS`.

## GO_TEST_SRCS
В макросе `GO_TEST_SRCS` должны быть явно перечислены все исходные файлы на языке `GO` (имена файлов должны оканчиваться на `_test.go`), в которых реализованы "внутренние" тесты.

{% note warning %}

Использование макроса `GO_TEST_SRCS` в модуле [GO_TEST_FOR](modules.md#go_test_for) строго не рекомендовано.

{% endnote %}

## GO_XTEST_SRCS
В макросе `GO_XTEST_SRCS` должны быть явно перечислены все исходные файлы на языке `GO` (имена файлов должны оканчиваться на `_test.go`), в которых реализованы "внешние" тесты.

{% note warning %}

Использование макроса `GO_XTEST_SRCS` в модуле [GO_TEST_FOR](modules.md#go_test_for) строго не рекомендовано.

{% endnote %}

## CGO_CFLAGS
В макросе `CGO_CGLAGS` должны быть перечислены дополнительные флаги компиляции `C` кода, необходимые для сборки `CGO`.

## CGO_LDFLAGS
В макросе `CGO_LDFLAGS` должны быть перечислены дополнительные флаги линковки `C` кода, необходимые для сборки `CGO`.

## GO_ASM_FLAGS
В макросе `GO_ASM_FLAGS` должны быть перечислены дополнительные флаги для компиляции `Asm` файлов, указанных в [SRCS](#srcs).

## GO_COMPILE_FLAGS
В макросе `GO_COMPILE_FLAGS` должны быть перечислены дополнительные флаги для команды `compile`.

### GO_LINK_FLAGS
В макросе `GO_LINK_FLAGS` должны быть перечислены дополнительные флаги для команды `link`.

## GO_SKIP_TESTS
Вызов макроса `GO_SKIP_TESTS` позволяет игнорировать тесты, имена которых перечислены в параметрах макроса. В параметрах макроса должны быть перечислены только 'верхнеуровневые' имена статических тестов. Имена тестов, перечисленные в макросе `GO_SKIP_TESTS`, будут отфильтрованы во время генерации тестовой обёртки.

{% note alert %}

Использование этого макроса разрешено только в проектах из корневой директории `vendor`. Этот макрос реализован для отключения тестов  внешних проектов на языке `GO` (с целью избежать модификации исходного кода внешних по отношению к Аркадии проектов). Для отключения тестов в других директориях нужно использовать стандартные средства `GO`.

{% endnote %}

## GO_PACKAGE_NAME
Вызов макроса `GO_PACKAGE_NAME` позволяет переопределить имя пакета по умолчанию. Параметр макроса определяет имя пакета. По умолчанию имя пакета определяется последним слогом пути до пакета (кроме пакета для модуля типа [GO_PROGRAM](modules.md#go_program) - в этом случае имя пакета всегда `main`).

## GO_PROTO_PLUGIN
[GO_PROTO_PLUGIN](../proto/macros.md#go_proto_plugin).

## GO_GRPC_GATEWAY_SRCS
Вызов `GO_GRPC_GATEWAY_SRCS` определяет набор `.proto` файлов для которых будет вызван gRPC-Gateway плагин для `protoc`. Этот плагин прочитает определение gRPC сервиса и по дополнительным аннотациям к rpc-определениям сгенерирует "обратный прокси" (reverse-proxy) сервер. Исходный код, сгенерированный плагином, будет записан в файлы с расширением `.pb.gw.go`. Чтобы посмотреть код сгенерированных файлов можно давить флаг `--add-result .pb.gw.go` в строке запуска `ya make ...`.

## GO_GRPC_GATEWAY_SWAGGER_SRCS
Вызов макроса `GO_GRPC_GATEWAY_SWAGGER_SRCS` полностью повторяет логику макроса [GO_GRPC_GATEWAY_SRCS](#go_grpc_gateway_srcs). В дополнение к сгенерированным файлам "обратного прокси" (reverse-proxy) сервера (файлы `.pb.gw.go`) будет также сгенерирована Swagger спецификация в формате `JSON`.

## GO_MOCKGEN_FROM

## GO_MOCKGEN_TYPES

## GO_MOCKGEN_REFLECT

## GO_MOCKGEN_MOCKS

## USE_CXX
Вызов `USE_CXX` добавляет в сборку модуля зависимость на `C++` runtime. По умолчанию сборка `Go` модулей не подразумевает подключение `C++` рантайма. Вызов этого макроса необходим, когда `CGO` коде модуля используется `C++` код.

## USE_UTIL
Вызов `USE_UTIL` добавляет в сборку модуля зависимость на `Аркадиёный` [util](https://a.yandex-team.ru/arc/trunk/arcadia/util) (а также `C++` runtime). Вызов этого макроса необходим, когда в `CGO` коде модуля используется `Аркадийный` [util](https://a.yandex-team.ru/arc/trunk/arcadia/util).

## Поддержка go:embed { #embed }

## GO_EMBED_PATTERN



## GO_TEST_EMBED_PATTERN


## GO_XTEST_EMBED_PATTERN