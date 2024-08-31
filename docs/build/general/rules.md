# Правила и проверки

## Общие правила

[Правила разработки в Arcadia](https://docs.yandex-team.ru/devtools/rules/intro)


## Ограничения, проверяемые системой сборки

### Ошибки в ya.make и не только

Во время своей работы система сборки не только обнаруживает ошибки в файлах `ya.make`. Она также может сообщить о том, что

- По указанному в зависимости пути нет сборочной цели (нет директории, нет `ya.make`, в `ya.make` не описан модуль)
- Нет указанного в макросе файла (символьные ссылки файлами не считаются)

и т.п. Обнаружение такого рода ошибок понятно и ожидаемо от системы сборки. Однако, во время анализа зависимостей система сборки также пытается проанализировать `include`/`import` и т.п.
Чтобы это сделать система сборки пытается понять какой файл имеется в виду в каждом `include`/`import`. Помогают ей в этом пути поиска, задаваемые макросом `ADDINCL`.

Если система сборки не может преобразовать имя в `#include`/`import` и т.п. в файл, то она выдаёт ошибку такого вида:
```
Error[-WBadIncl]: in $B/devtools/examples/diag/ymake/bad_incl/bad_incl: could not resolve include file: bits/alltypes.h included from here: $S/contrib/libs/musl/include/stddef.h
```

Здесь важно понимать, что поиск не обязательно делается 
в контексте того модуля, которому принадлежит файл с `#include` —  он начинается от файла упомянутого в `ya.make` и делается рекурсивно на всю глубину по `#include` (компилятор работает также).
Это значит, что он может переходить по хедерам в другие модули, но ADDINCL будет использован из исходного модуля. Чтобы необходимые `ADDINCL` оказались в месте использования необходимо

- Всегда ставить `PEERDIR` для того, что используется по `#include`
- Если для превращения имён в пути в хедере нужен `ADDINCL`, такой `ADDINCL` может быть помечен как `GLOBAL`, тогда он по зависимостям `PEERDIR` будет доступен во всех зависимых модулях.

Поскольку наш анализ зависимостей не учитывает `#ifdef` (для скорости и однократности в рамках сборки), то необходимо обеспечить безусловное преобразование имён в файлы либо предоставив необходимый 
`ADDINCL`, либо через механизм `sysincl`. 

### Запрещённые и ограниченные зависимости

В Аркадии не любые зависимости разрешены. Первые три пункта ниже контролируются [специальными правилами](../manual/common/peerdir_rules.md).

1. Часть общих библиотек не рекомендована к использованию. Новые зависимости на такие библиотеки запрещены правилами. Часть кода проекты в Аркадии считают деталями реализации и его использование также запрещено правилами.
   Пример ошибки:
   ```
   Error[-WBadDir]: in $B/devtools/examples/diag/ymake/bad_dir_peerdir_policy/bad_dir_peerdir_policy: PEERDIR from $S/devtools/examples/diag/ymake/bad_dir_peerdir_policy to $S/contrib/restricted/abseil-cpp is prohibited by peerdir policy
   ```

2. Считается, что директория `internal` — это детали реализации. Зависимости на неё можно иметь только её *братьям* и их *детям* по дереву исходного кода (т.е. только *детям* её родительской директории).

3. Часть кода исключено из сборки совсем (относится к проектам, используемым в Аркадии с другими системами сборки).

4. Часть модулей не совместимы по зависимостям. Наиболее яркий пример —  это модули на 2м и 3м питоне, которые не могут иметь зависимостей друг на друга.
   Пример ошибки:
   ```
   Error[-WBadDir]: in $B/devtools/examples/diag/ymake/bad_dir_incompatible_tags/libpydiag-ymake-bad_dir_incompatible_tags.a: PEERDIR from module tagged PY3 to $S/devtools/examples/diag/ymake/bad_dir_incompatible_tags/proto is prohibited: tags are incompatible
   ```

### Include без PEERDIR

В Аркадии каждый `#include`/`import` должен быть подкреплён зависимостью `PEERDIR`. Сейчас это правило почти не контролируется и часто нарушается. `#include`/`import` от корня Аркадии не создают видимых проблем
с обнаружением зависимых файлов для самой системы сборки. Однако, в итоге они могут приводить к ошибкам сборки, линковки или исполнения. Есть планы по усиленю контроля в этом направлении.

Одна такого рода проверка существует уже сейчас. Выполняется она только в CI и она проверяет наличие `PEERDIR` только при использовании генерированного кода. Диагностика выглядит так:

```
used a file $B/market/idx/datacamp/proto/offer/OfferMeta.pb.h belonging to directories ($S/market/idx/datacamp/proto) which are not reachable by PEERDIR
[ Guess]: PEERDIR is probably missing: $S/search/panther/custom/dispatch -> $S/search/panther/custom/market_v0_13
[ Guess]: PEERDIR is probably missing: $S/search/panther/custom/market_v0_13 -> $S/search/panther/custom/market_common
[  Path]: $B/search/panther/tools/idx_convert/lib/libtools-idx_convert-lib.a ->
[  Path]: $B/search/panther/custom/dispatch/libpanther-custom-dispatch.a ->
[  Path]: $B/search/panther/custom/dispatch/dummy.cpp.o ->
[  Path]: $S/search/panther/custom/dispatch/dummy.cpp ->
[  Path]: $S/search/panther/custom/dispatch/dispatch.h ->
[  Path]: $S/search/panther/custom/market_v0_13/panther_market_v0_13.h ->
[  Path]: $S/search/panther/custom/market_v0_13/formula_market_v0_13.h ->
[  Path]: $S/search/panther/custom/market_common/market_aux_reader.h ->
[  Path]: $S/market/report/library/base_search_document_basic_props/base_search_document_basic_props.h ->
[  Path]: $S/market/idx/library/proto_helpers/offers_data.h ->
[  Path]: $B/market/idx/datacamp/proto/offer/OfferMeta.pb.h
```

Чтобы включить такую диагностику при локальной сборке необходимо использовать параметр `--warning-mode ChkPeers`. Починка такого рода проблем может быть сложна, а без починки их всех devtools не может
включить проверку по умолчанию локально. Это бы сломало сборку большому количеству разработчиков в Аркадии - всем, кто зависит от проблемных библиотек.

### Лицензии

Система сборки умеет контролировать лицензии у сборочных целей. Для этого все библиотеки, имеющие специальные лицензии размечаются макросом [`LICENSE`](../manual/common/macros.md#license).

В сборочных целях (обычно программах или динамических библиотеках) распространяемых из Аркадии ставится макрос [`RESTRICT_LICENSES`](../manual/common/macros.md#restrict_licenses) или его производные.
Система сборки транзитивно замыкает лицензии всех зависимостей контролируемой программы и проверяет, что в полученном списке нет нарушений.

### Транзитивные межмодульные инварианты

Аналогично проверкам лицензий система сборки может делать и другой контроль за транзитивными зависимостями сборочных целей

  - [`PROVIDES`](../manual/common/macros.md#provides) позволяет исключить использование в одной программе разных библиотек с одним и тем же интерфейсом, чтобы избежать конфликтов.
    Библиотеки, про которые известно, что для них есть несколько конфликтующих вариантов, размечаются макросом [`PROVIDES(<feature>)`](../manual/common/macros.md#provides). Контроль делается в модулях типа
    `PROGRAM` и т.п.
  - [`CHECK_DEPENDENT_DIRS`](../manual/common/macros.md#check_dependent_dirs) позволяет ограничить зависимости модуля чёрным или белым списком по именам (директориям) модулей.

  - В Java модулях можно использовать [`JAVA_DEPENDENCIES_CONFIGURATION`](../manual/java/macros.md#java_dependencies_configuration) для контроля за правилами по которым выбираются версии сторонних библиотек при попадании нескольких версий одной и той же библиотеки в транзитивное замыкание зависимостей модуля.

### Валидация кода и проверки стиля

Система сборки умеет генерировать валидационные тесты и проверки стиля. Такие тесты не надо описывать, при запуске `ya make -t` для модуля проверки будут добавлены автоматически в зависимости от типа модуля или использованных макросов.


{% list tabs %}

- Python

  - Для программ и библиотек на Python добавляются проверки стиля `flake8` для каждого исходного файла, добавленного в сборку с помощью `PY_SRCS`. Конфиг `flake8` находится [тут](https://a.yandex-team.ru/arc_vcs/build/config/tests/flake8/flake8.conf)
  - Для программ на Python дополнительно добавляется `import_test` - проверки импортируемости внутренних пакетов во всём их коде, включая зависимости.
  
- Go

  - Для модулей на go добавляются проверки `go vet` с настройками, описанными [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yolint/cmd/yolint/main.go)
  - Для исходного кода на go добавляются тесты стиля `go fmt`

- Java

  - Для модулей на Java добавляются тесты стиля для каждого исходного файла
  - Для модулей на Java добавляются проверки непротиворечивости classpath (classpath clash)

- Другое

  - При использовании макросов, использующих ресурсы, таких как [`FROM_SANDBOX`](../manual/common/data.md#from_sandbox) или [`LARGE_FILES`](../manual/common/data.md#large_files) добавляется валидация наличия и времени жизни использованных ресурсов.

{% endlist %}
      
