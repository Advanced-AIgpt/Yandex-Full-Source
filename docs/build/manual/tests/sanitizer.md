# Тесты с Sanitizer

`ya make -t --sanitize X` позволяет собирать инструментированные программы с санитайзерами. Сейчас поддерживаются: `address`, `memory`, `thread`, `undefined`, `leak`. Можно указывать только один санитайзер за раз.

## Параметризация опций санитайзеров {#local}
Локальный запуск `ya make -t --sanitize X` учитывает [стандартные опции](https://github.com/google/sanitizers/wiki/SanitizerCommonFlags) для санитайзеров переданные через переменные окружения.
По умолчанию тестовая машинерия добавляет всем санитайзерам опцию `exitcode=100` для специальной обработки падений тестов в этой конфигурации.
В `UBSAN_OPTIONS` дополнительно выставляются опции `print_stacktrace=1,halt_on_error=1`.
Вы можете зафиксировать опции санитайзеров для конкретных тестов через макрос `ENV()` в ya.make, например: `ENV(ASAN_OPTIONS=detect_stack_use_after_return=1)`

## Cборка с санитайзером в Sandbox {#sandbox}
Для сборки следует использовать задачу `YA_MAKE`/`YA_MAKE2`, в которой можно выбрать требуемый санитайзер в поле `Build with specified sanitizer`

## Запуск тестов c санитайзером в Автосборке {#autocheck}
В автосборке подключены 2 типа cанитайзеров: [memory и address](https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/linux/ya.make?rev=6748504#L27-37), так как они достаточно легковесны и стабильны.
Для подключения проекта ко всем типам cанитайзеров следует добавить свой проект в [autocheck/linux/sanitizer_common_targets.inc](https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/linux/sanitizer_common_targets.inc)
Если требуется только `memory` санитайзер, то в [autocheck/linux/sanitizer_memory_targets.inc](https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/linux/sanitizer_memory_targets.inc)
Если требуется только `address` санитайзер, то в  [autocheck/linux/sanitizer_address_targets.inc](https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/linux/sanitizer_address_targets.inc)

## Отключение тестов от сборки с санитайзерами {#disable}
1. Вы можете сделать проект не достижимым по рекурсам для санитайзеров, подключив проекты более гранулярно. См. предыдущий абзац.
2. Вы можете сделать проект не достижимым по рекурсам для санитайзеров, например:
    ```
    IF (NOT SANITIZER_TYPE)
        RECURSE_FOR_TESTS(
            test
        )
    ENDIF()
    ```
3. Отключить конкретный тест с помощью тега `ya:not_autocheck`, например:
    ```
    IF (SANITIZER_TYPE)
        TAG(ya:not_autocheck)
    ENDIF()
    ```
4. Отключить инструментирование конкретной библиотеки c помощью макроса `NO_SANITIZE()`
5. С помощью макроса `SUPPRESSIONS()` можно указать файл содержащий правила для подавления ошибок в [стандартной нотации](https://clang.llvm.org/docs/AddressSanitizer.html#suppressing-memory-leaks) с поддержкой комментариев начинающихся с `#`. Механизм поддерживается для `address`, `leak` и `thread` санитайзеров.
Пример для protobuf: [ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/protobuf/ya.make?rev=7322093#L17) [tsan.supp](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/protobuf/tsan.supp?rev=7322093)

{% note warning %}

Добавлять исключения следует только если вы переносите известные исключения из контриба или отчётливо понимаете, что сообщение от санитайзеров ложноположительное (скорей всего нет и вам следует внимательней разобраться в проблеме). Каждое обновление кода или компилятора должно приводить к пересмотру suppression списка.

{% endnote %}

