# Задача YA_MAKE_2

Задача для сборки и тестирования с помощью [ya make](../usage/ya_make/).

[Исходный код задачи](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/common/build/YaMake2)

[YaMakeKosher](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/common/build/KosherYaMake/__init__.py) - отличается от оригинального YaMake2 только другими значениями по умолчанию некоторых параметров. Включены distbuild и arcadia aapi по умолчанию. Также добавлен Mixin ReleaseToNannyTask2 для интеграции с няней. Существуют трудности с включением этих параметров в YaMake, потому что они не поддержаны на всех платформах. Новые фичи по ускорению сборки по умолчанию появляются в этой таске.

## Опции
* **Svn url for arcadia** (`checkout_arcadia_from_url`)  
  Указатель на Аркадию в формате Arcadia URL. Поддерживаются SVN и arc.

  Для SVN надо использовать схему `arcadia:`. Возможные варианты:
  * `arcadia:/arc/trunk/arcadia` — trunk, **значение по умолчанию**
  * `arcadia:/arc/trunk/arcadia@123` — версия с заданной SVN-ревизией
  * `arcadia:/arc/branches/some/branch/arcadia` — версия из заданной SVN-ветки

  Для arc надо использовать схему `arcadia-arc:`. Возможные варианты:
  * `arcadia-arc:/#trunk` — trunk
  * `arcadia-arc:/#r123` — версия с заданной SVN-ревизией
  * `arcadia-arc:/#dae86e1950b1277e545cee180551750029cfe735` — версия с заданной arc-ревизией
  * `arcadia-arc:/#users/volozh/international-expansion` — версия из заданной arc-ветки
  * поддерживаются [и другие способы указать на ревизию в arc](https://docs.yandex-team.ru/arc/ref/commands#commit-hash), итоговый URL имеет формат `arcadia-arc:/#<указатель>`

* **Build bundle (multiple archs)** (`build_bundle`)  
Является ли задача мета-задачей для сборки под все архитектуры

* **Apply patch** (`arcadia_patch`)  
Патч, который будет наложен на Аркадию. Подробнее про то, как подготовить правильный патч, можно почитать [тут](https://wiki.yandex-team.ru/users/v-korovin/patch/).

### Project configuration parameters
В процессе сборки таска создает (всегда) ресурс BUILD_OUTPUT, куда складываются все артефакты сборки, а также, если заполнены поля build/source artifacts - дополнительный ресурс с описанным в данных полях содержанием. Таким образом можно собирать ресурсы с необходимой структурой директорий.

* **Targets** (`targets`)  
Какие цели собирать (разделитель `;`)  

* **Build artifacts (semicolon separated pairs path[=destdir])** (`arts`)  
Какие артефакты сборки поместить в отдельный ресурс: пути от корня Аркадии (например: `tools/printcorpus/printcorpus` - бинарник утилиты), через знак равенства можно указать директорию в ресурсе, куда будет сложен файл (`=bin`)  , по-умолчанию используется структура ресурса, аналогичная расположению артефактов в дереве Аркадии.

* **Source artifacts (semicolon separated pairs path[=destdir])** (`arts_source`)  
Какие файлы из Аркадии поместить в отдельный ресурс (формат тот же, что и у build artifacts).

* **Result is a single file** (`result_single_file`)  
Если в полях описания артефактов указан единственный путь, то его можно поместить в корень создаваемого отдельного ресурса с помощью этой опции. Например, так можно собирать ресурс с единственным бинарником.

* **Result resource type** (`result_rt`)  
Тип создаваемого таской отдельного ресурса

* **Result resource description** (`result_rd`)  
Описание для создаваемого отдельного ресурса

* **Use selective checkout** (`checkout`)  
Запускать `ya make` с [селективным чекаутом](https://docs.yandex-team.ru/ya-make/usage/ya_make/local#selective_co)

### Build system params

* **Build system** (`build_system`)  
  Режим сборки:
    * `ya` - локальная сборка в Сандбоксе;
    * `semi-distbuild` - конкурентная сборка локальная и с использованием дистбилда (какая первая завершится);
    * `distbuild` - сборка на дистбилде.

* **Build type** (`build_type`)  
Профиль сборки, содержимого пакета.  
По умолчанию: `release`

* **Definition flags** (`definition_flags`)  
Флаги сборки, например, `-DUSE_ARCADIA_PYTHON=no`

* **Check "ya make" return code** (`check_return_code`)  
Верифицировать код завершения `ya make`, в этом случае задача будет перейдет в состояние `FAILURE` при ошибках сборки

* **Build and test as much as possible** (`keep_on`)  
Ключ `--keep-going (-k)` для запуска `ya make`

* **Build by test's DEPENDS anyway** (`force_build_depends`)  
Собирать зависимости тестов

* ~~**Target platform**~~
Таргет-платформа, устаревшее, вместо него нужно использовать поле `target_platform_flags`

* **Target platform flags** (`target_platform_flags`)  
Флаги таргет-платформ **только** для кросс-компиляции

* **Build with LTO** (`lto`)  
Собирать с Link Time Optimization

* **Create PGO profile** (`pgo_add`)  
Создать профиль PGO после выполнения задачи ([подробнее](https://wiki.yandex-team.ru/yatool/make/#pgo))

* **Use PGO profiles** (`pgo_use`)  
При сборке использовать выбранный профиль PGO ([подробнее](https://wiki.yandex-team.ru/yatool/make/#pgo))

* **Timeout for llvm-profdata tool (merge PGO raw profiles)**  (`pgo_merge_timeout`)  
Таймаут на запуск `llvm-profdata` при создании профиля PGO

* **Timeout for ya command** (`ya_timeout`)  
Таймаут на запуск `ya make` в секундах

* **Use system Python to build python libraries** (`use_system_python`)  
Использовать системный питон при сборке — важно, если собираются so-файлы для последующей работы в окружении с системным питоном

### Base build params
* ~~**Use dev version of build system**~~
Устравшее

* **Clear build** (`clear_build`)  
Не использовать кеш сборки от предыдущих запусков задачи

* **Strip debug information (i.e. strip -g)** (`strip_binaries`)  
Нужно ли удалять отладочную информацию из бинарного файла

### Environment params
* **Vault owner** (`vault_owner`)  

* **Vault key name** (`vault_key_name`)  

### Optional params
* **Use ya bloat for specified binaries** (`use_ya_bloat`)  
Для собираемых целей запустить [ya bloat](https://wiki.yandex-team.ru/yatool/bloat/) (разделитель `;`)  

* **Check build dependencies** (`check_dependencies`)  
Сгенерировать список зависимостей для собираемых целей, если какие-то из них в бан-листе, задача упадет [SEARCH-1578](https://st.yandex-team.ru/SEARCH-1578)

### Test system params
* **Run tests** (`test`)  
Запускать тесты

* **Test threads (no limit by default)** (`test_threads`)  
Ограничение на количество одновременно запускаемых тестов

* **Test parameters** (`test_params`)  
Параметры тестов, передаваемые тестам (`--test-param`)  , например `key1=val1 ... keyN=valN`

* **Test filters** (`test_filters`)  
Фильтры для запуска тестов по имени теста (например `TUtilUrlTest::TestSchemeGet TUtilUrlTest::TestGetZone`)  

* **Filter tests by size** (`test_size_filter`)  
Фильтр для запуска тестов определенного размера

* **Run tests that have specified tag** (`test_tag`)  
Фильтр для запуска тестов с определенными метками

* **Filter tests by suite type** (`test_type_filter`)  
Фильтр для запуска тестов определенного типа (PY2TEST/UNITTEST/JUNIT) (разделитель `;`)  

* **Generate allure report** (`allure_report`)  
Создавать allure-отчет с результатами тестирования (отдельным ресурсом)

* **Allure report TTL** (`allure_report_ttl`)  
Время жизни в днях для ресурса с allure-отчетом 

* **Keep only test entries in report** (`report_tests_only`)  
В отчете для ТЕ оставлять только записи с результатами тестов

* **Add junit report for tests** (`junit_report`)  
Генерировать junit-отчет в папке результатов сборки (ресурс BUILD_OUTPUT)

* **Specifies logging level for output test logs** (`test_log_level`)  
Уровень логирования для тестов

* **Tests coverage** (`tests_coverage`)  
Генерировать отчет о покрытии (отдельный ресурс после выполнения задачи)

* **Tests coverage prefix filter** (`coverage_prefix_filter`)  
Фильтр для создания отчета о покрытии, например, `devtools/ya` - если интересует покрытие только определенных участков репозитория

* **Exclude matching files from coverage report** (`coverage_exclude_regexp`)  
Исключить файлы определенных типов из покрытия

* **Build with specified sanitizer** (`sanitize`)  
Сборка с санитайзером

* **Turn off timeout for tests** (`disable_test_timeout`)  
Отключить таймауты для запуска тестов - бывает полезным при сборке с санитайзерами. Работает **только** c локальной сборкой, несовместимо с `--cache-tests` и `--dist`

* **Cache test results** (`cache_test_results`)  
Кешировать результаты тестов (последующие запуски, попадающие в кеш, фактически запускаться не будут, а их результаты будут взяты из кеша)

* **Ram drive size (Mb)** (`ram_drive_size`)  
Размер рам-диска - используется в сочетании с требованиями таски предоставить рам-диск определенного размера в FAT-контуре

* **Tests retries** (`tests_retries`)  
Количество запусков тестов, числа > 1 используются для обнаружения нестабильных тестов, все результаты запусков затем агрегируются в один с анализом под-запусков - если тест вел себе по-разному, то считается нестабильным (FLAKY). В автосборке используется значение = 2

* **Tests failure exit code** (`test_failure_code`)  
Код завершения `ya make` при наличии упавших тестов

### Java params
* **JVM args** (`jvm_args`)  
Параметр `--jvm-args` запуска `ya make`


## YaMakeTemplate
Анонс технологии [в Этушке](https://clubs.at.yandex-team.ru/arcadia/20318).

[YaMakeTemplate](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/common/build/tasks/YaMakeTemplate/__init__.py)  - задача-наследник `YA_MAKE_2`
для простого конструирования собственных сборочных задач, "заточенных" под определённый список собираемых целей.

Зная тип собираемых ресурсов, можно получить сборочную задачу в 3 строчки, например,
[BuildStokerYa](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/stoker/BuildStokerYa/__init__.py?rev=5624483#L6-11).

Для корректной работы нужно, чтобы в собираемых ресурсах был прописан атрибут
[arcadia_build_path](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/stoker/resource_types.py?rev=5624483#L21),
а в параметры был передан список ресурсов, [вот так](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/stoker/BuildStokerYa/__init__.py?rev=5624483#L10).

В случае, если параметры не переданы, либо у одного из ресурсов нет `arcadia_build_path`, задача будет переходить в `FAILURE`.

Кроме таргетов сборки, можно также пакетировать исходные файлы из Аркадии. Например, таким образом удобно пакетировать конфигурационные файлы.
Для этого используется второй аргумент `source_resources` метода `get_project_params`. Актуальный пример использования можно увидеть
в docstring [самого метода](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/common/build/tasks/YaMakeTemplate/__init__.py).

Все остальные параметры сборки можно менять так же, как и у обычной задачи `YA_MAKE_2`.

В результате мы получим задачу с возможностью ставить checkbox-ы напротив нужных нам целей, которые надо собрать (по умолчанию собираются все перечисленные цели).

### FAQ
**Q**: У меня была своя сборочная задача, я перевёл её на базу `YaMakeTemplate`, собрал локально (чтобы протестировать) и запустил в Sandbox,
но ничего не работает, хотя сделал всё так же, как описано в примере.
**A**: Так как задача формирует список целей в `on_enqueue`, код этого метода должен "доехать" до Sandbox. Поэтому следует закоммитить изменения
и дождаться их выкладки в Sandbox. Подробнее см. [RMDEV-2388](https://st.yandex-team.ru/RMDEV-2388).

**Q**: У меня какая-то другая проблема с `YaMakeTemplate`, не описанная здесь.
**A**: Обратитесь в чат поддержки [Release Machine Support](https://t.me/joinchat/Q3j3Bey38NI5OxBo), мы попробуем вам помочь.
