# ya dump : получение информации из системы сборки и репозитория


Команды `ya dump` выводят информацию о сборочном графе, а также позволяют получить информацию о системе сборки. 

Первая группа команд может быть полезна для анализа зависимостей между модулями, а также для поиска проблем. К ней относятся

* [ya dump modules](#modules)  – список зависимых модулей
* [ya dump relation](#relation)  – зависимость между двумя модулями
* [ya dump all-relations](#all-relations)  – все зависимости между двумя модулями
* [ya dump dot-graph](#dot-graph) – граф всех межмодульных зависимостей данного проекта
* [ya dump dep-graph, ya dump json-dep-graph](#dep-graph)  – граф зависимостей системы сборки
* [ya dump build-plan](#build-plan) – граф сборочных команд
* [ya dump loops, ya dump peerdir-loops](#loops) – информация о циклах в графе зависимостей
* [ya dump compile-commands, ya dump compilation-database](#compile) – информация о сборочных командах (compilation database).

{% note info %}

По умолчанию вывод этих команд основан на графе **обычной сборки** (ака `ya make`, т.е. без тестов). Для поиска информации с учётом сборки тестов (ака `ya make -t`)
надо добавить аналогичную опцию, например `ya dump modules -t`.

{% endnote %}


Вторая группа - это различная информация от системы сборки. К ней относятся:

* [ya dump groups](#groups) – группы владельцев проектов
* [ya dump json-test-list](#test-list) – информация о тестах
* [ya dump recipes](#recipes) – информация о поднимаемых рецептах
* [ya dump conf-docs](#docs) - документация по макросам и модулям
* [ya dump debug](#debug) — сборка отладочного bundle
* [ya dump --help](#help) - справка по командам `ya dump`


## ya dump для анализа зависимостей

Многие команды семейства `ya dump` предназначены для поиска и анализа зависимостей проектов в Аркадии. Ниже описаны наиболее распространённые сценарии для управления зависимостями в
проектах с помощью фильтров в командах `ya dump modules` и `ya dump relation`.

{% cut "Зачем фильтровать зависимости?" %}

В _графе зависимостей_ системы сборки представлен разнообразные зависимости, в том числе между проектами (`PEERDIR`) и директориями (`RECURSE`), получаемые из ya.make-файлов и исходного кода.
Ymake использует их, чтобы построить граф команд сборки для разных ситуаций, но при этом часть информаци о зависимостях теряется.
Поэтому `ya dump` собирает информацию обходом _графа зависимостей_ системы сборки, а чтобы смоделировать ту или иную ситуацию, нужно исключать из обхода графа разные виды зависимостей.

{% note info %}

Ymake умеет делать `DEPENDENCY_MANAGAMENT` для Java на основе _графа зависимостей_ поэтому всё описанное далее применимо к Java также как и к остальным языкам в Аркадии.

{% endnote %}

{% endcut %}



### Что участвует в сборке

Узнать, что будет собираться если написать `ya make <path>`/`ya make -t <path>`.

{% cut "Правила обхода" %}

* Целями сборки будут: ваш модуль + все модули, достижимые от вашего только по `RECURSE`.
* Для всех целей соберутся также все модули, от которых они зависят, то есть достижимые от них по `PEERDIR`, включая программы, которые используются для кодогенерации и запускаемые через `RUN_PROGRAM`.
* `RECURSE` от зависимых модулей в сборке не участвуют,
* C опцией `-t` или `--force-build-depends` к целям сборки добавятся тесты, достижимые по `RECURSE_FOR_TESTS`, а также  `DEPENDS` - зависимости ваших тестов.

{% endcut %}

1. Список всех зависимостей для проекта: `ya dump modules`
   ```
   ~/ws/arcadia$ ya dump modules devtools/ymake
   module: Library devtools/ymake $B/devtools/ymake/libdevtools-ymake.a <++ SELF
   ...
   module: Library util $B/util/libyutil.a <++ PEERDIRs
   ...
   module: Program devtools/ymake/bin $B/devtools/ymake/bin/ymake <++ RECURSEs
   ... 
   module: Program contrib/tools/python3/pycc $B/contrib/tools/python3/pycc/pycc   <++ TOOLs
   ...
   ```
   {% note tip %}

   Если хочется видеть и зависимости тестов тоже, используйте `-t` или `--force-build-depends`

   ```
   ~/ws/arcadia$ ya dump modules devtools/ymake | wc -l
   861
   ~/ws/arcadia$ ya dump modules devtools/ymake -t | wc -l
   1040
   ```

   {% endnote %}

2. Список всех зависимостей для проекта в директории `<dir>`: `ya dump modules | grep <dir> `
   ```
   ~/ws/arcadia$ ya dump modules devtools/ymake | grep mapreduce/
   module: Library mapreduce/yt/unwrapper $B/mapreduce/yt/unwrapper/libpymapreduce-yt-unwrapper.a
   module: Library mapreduce/yt/interface $B/mapreduce/yt/interface/libmapreduce-yt-interface.a
   module: Library mapreduce/yt/interface/protos $B/mapreduce/yt/interface/protos/libyt-interface-protos.a
   module: Library mapreduce/yt/interface/logging $B/mapreduce/yt/interface/logging/libyt-interface-logging.a
   module: Library mapreduce/yt/client $B/mapreduce/yt/client/libmapreduce-yt-client.a
   module: Library mapreduce/yt/http $B/mapreduce/yt/http/libmapreduce-yt-http.a
   module: Library mapreduce/yt/common $B/mapreduce/yt/common/libmapreduce-yt-common.a
   module: Library mapreduce/yt/io $B/mapreduce/yt/io/libmapreduce-yt-io.a
   module: Library mapreduce/yt/raw_client $B/mapreduce/yt/raw_client/libmapreduce-yt-raw_client.a
   module: Library mapreduce/yt/skiff $B/mapreduce/yt/skiff/libmapreduce-yt-skiff.a
   module: Library mapreduce/yt/library/table_schema $B/mapreduce/yt/library/table_schema/libyt-library-table_schema.a
   ```

   {% note info %}

   * Это может быть полезно если точная зависимость неизвестна.
   * `ya dump modules` включает и модули самого проекта, поэтому подав путь до проекта в `<dir>` можно узнать как называются модули в мультимодуле

   {% endnote %}


3. Каким образом проект зависит от модуля: `ya dump relation '<module_name>'`
   ```
   ~/ws/arcadia$ cd devtools/ymake
   ~/ws/arcadia/devtools/ymake$ ya dump relation mapreduce/yt/interface
   Directory (Start): $S/devtools/ymake/tests/dep_mng ->
   Program (Include): $B/devtools/ymake/tests/dep_mng/devtools-ymake-tests-dep_mng ->
   Library (BuildFrom): $B/devtools/ya/test/tests/lib/libpytest-tests-lib.a ->
   Library (Include): $B/devtools/ya/lib/libya-lib.a ->
   Library (Include): $B/devtools/ya/yalibrary/store/yt_store/libpyyalibrary-store-yt_store.a ->
   Library (Include): $B/mapreduce/yt/unwrapper/libpymapreduce-yt-unwrapper.a ->
   Directory (Include): $S/mapreduce/yt/interface
   ```
   {% note info %}

   * Команду надо запускать в директории проекта, от которого ищем путь.
   * module_name - имя модуля как пишет `ya dump modules` или директория проекта этого модуля. 
   * Будет показан один из возможных путей.

   {% endnote %}

   {% note tip %}

   * В случае, если хочется найти зависимость от конкретного модуля в мультимодуле надо подсмотреть полное имя модуля в `ya dump modules` и передать его в параметре `--from`.
   * В случае, если хочется найти зависимость от неизвестного модуля где-то в директории `<dir>`, можно использовать (2.) чтобы узнать от каких модулей в директори есть зависимости.

   {% endnote %}


4. Каким образом проект зависит от модуля (все пути): `ya dump all-relations '<module_name>'`

   {% note info %}

   * Команду надо запускать в директории проекта, от которого ищем путь.
   * module_name - имя модуля как пишет `ya dump modules` или директория проекта этого модуля. 
   * На выходе будет граф в формате dot со всеми путями в интересующий модуль.

   {% endnote %}


### Что соберётся при `PEERDIR`

Узнать, что будет собираться через библиотеку, если поставить на неё `PEERDIR`.

{% cut "Правила обхода" %}

Здесь всё как выше, но надо исключить `RECURSE` совсем - при `PEERDIR` на модуль, его `RECURSE` не попадают в сборку.

{% endcut %}

1. список для проекта: `ya dump modules --ignore-recurses`
   ```
   ~/ws/arcadia$ ya dump modules devtools/ymake | wc -l
   861
   ~/ws/arcadia$ ya dump modules devtools/ymake --ignore-recurses | wc -l
   222
   ~/ws/arcadia$ ./ya dump modules devtools/ymake --ignore-recurses | grep mapreduce
   ~/ws/arcadia$ ./ya dump modules devtools/ymake --ignore-recurses | grep python
   module: Library contrib/libs/python $B/contrib/libs/python/libpycontrib-libs-python.a
   module: Library contrib/libs/python/Include $B/contrib/libs/python/Include/libpylibs-python-Include.a
   module: Library contrib/tools/python/lib $B/contrib/tools/python/lib/libtools-python-lib.a
   module: Library contrib/tools/python/base $B/contrib/tools/python/base/libtools-python-base.a
   module: Library contrib/tools/python/include $B/contrib/tools/python/include/libtools-python-include.a
   module: Program contrib/tools/python/bootstrap $B/contrib/tools/python/bootstrap/bootstrap
   module: Library library/python/symbols/module $B/library/python/symbols/module/libpypython-symbols-module.a
   module: Library library/python/symbols/libc $B/library/python/symbols/libc/libpython-symbols-libc.a
   module: Library library/python/symbols/registry $B/library/python/symbols/registry/libpython-symbols-registry.a
   module: Library library/python/symbols/python $B/library/python/symbols/python/libpython-symbols-python.a
   module: Library library/python/symbols/uuid $B/library/python/symbols/uuid/libpython-symbols-uuid.a
   module: Library library/python/runtime $B/library/python/runtime/libpylibrary-python-runtime.a
   module: Library contrib/python/six $B/contrib/python/six/libpycontrib-python-six.a
   ```


2. путь непосредственно от проекта в директории, до другого модуля  `ya dump relation --ignore-recurses <module_name>` 
   ```
   ~/ws/arcadia$ cd devtools/ymake
   ~/ws/arcadia/devtools/ymake$ ya dump relation --ignore-recurses mapreduce/yt/interface
   Target 'mapreduce/yt/interface' is not found in build graph.
   ~/ws/arcadia/devtools/ymake$ ya dump relation --ignore-recurses contrib/tools/python/bootstrap
   Directory (Start): $S/devtools/ymake ->
   Library (Include): $B/devtools/ymake/libdevtools-ymake.a ->
   Library (Include): $B/contrib/libs/python/libpycontrib-libs-python.a ->
   Library (Include): $B/contrib/libs/python/Include/libpylibs-python-Include.a ->
   Library (Include): $B/contrib/tools/python/lib/libtools-python-lib.a ->
   NonParsedFile (BuildFrom): $B/contrib/tools/python/lib/python_frozen_modules.o ->
   NonParsedFile (BuildFrom): $B/contrib/tools/python/lib/python_frozen_modules.rodata ->
   BuildCommand (BuildCommand): 2964:RUN_PROGRAM=([ ] [ PYTHONPATH=contrib/tools/python/src/Lib ] [ bootstrap.py python-libs.txt  ... ->
   Directory (Include): $S/contrib/tools/python/bootstrap
   ```

3. все пути непосредственно от проекта в директории, до другого модуля  `ya dump all-relations --ignore-recurses <module_name>` 


### Что попадёт в программу

Здесь нас интересуют именно библиотеки. Либо все, которые попадут в программу (если анализируем `PROGRAM` и т.п.), либо те, которые попадут при `PEERDIR` на `LIBRARY`.

{% cut "Правила обхода" %}

* Целью сборки является ваш модуль (`PROGRAM` или `LIBRARY`) и всё, от чего он зависит по `PEERDIR`
* То, что достижимо по `RECURSE` будет собираться, но линковаться будет в собственные библиотеки и программы и к вам не попадёт.
* Не будут линковаться к вам и сборочные инструменты.

{% endcut %}

1. список для проекта: `ya dump modules --ignore-recurses --no-tools`
   ```
   ~/ws/arcadia$ ya dump modules devtools/ymake --ignore-recurses | wc -l
   222
   ~/ws/arcadia$ ya dump modules devtools/ymake --ignore-recurses --no-tools | wc -l
   198
   ~/ws/arcadia$ ya dump modules devtools/ymake --ignore-recurses --no-tools | grep python
   module: Library contrib/libs/python $B/contrib/libs/python/libpycontrib-libs-python.a
   module: Library contrib/libs/python/Include $B/contrib/libs/python/Include/libpylibs-python-Include.a
   module: Library contrib/tools/python/lib $B/contrib/tools/python/lib/libtools-python-lib.a
   module: Library contrib/tools/python/base $B/contrib/tools/python/base/libtools-python-base.a
   module: Library contrib/tools/python/include $B/contrib/tools/python/include/libtools-python-include.a
   module: Library library/python/symbols/module $B/library/python/symbols/module/libpypython-symbols-module.a
   module: Library library/python/symbols/libc $B/library/python/symbols/libc/libpython-symbols-libc.a
   module: Library library/python/symbols/registry $B/library/python/symbols/registry/libpython-symbols-registry.a
   module: Library library/python/symbols/python $B/library/python/symbols/python/libpython-symbols-python.a
   module: Library library/python/symbols/uuid $B/library/python/symbols/uuid/libpython-symbols-uuid.a
   module: Library library/python/runtime $B/library/python/runtime/libpylibrary-python-runtime.a
   module: Library contrib/python/six $B/contrib/python/six/libpycontrib-python-six.a
   ```

2. путь непосредственно от проекта в директории, до другого модуля: `ya dump relation --no-all-recurses --no-tools <module_name>`
   ```
   ~/ws/arcadia$ cd devtools/ymake
   ~/ws/arcadia/devtools/ymake$ ya dump relation --ignore-recurses --no-tools contrib/tools/python/bootstrap
   Sorry, path not found
   ~/ws/arcadia/devtools/ymake$ ya dump relation --ignore-recurses --no-tools contrib/tools/python/lib
   Directory (Start): $S/devtools/ymake ->
   Library (Include): $B/devtools/ymake/libdevtools-ymake.a ->
   Library (Include): $B/contrib/libs/python/libpycontrib-libs-python.a ->
   Directory (Include): $S/contrib/tools/python/lib
   ```

3. все пути непосредственно от проекта в директории, до другого модуля  `ya dump all-relations --ignore-recurses --no-tools <module_name>` 


## Описание команд

### Общие опции

Опции команд `ya dump` во многом аналогичны опциям `ya make`. Актуальный список опций можно посмотреть с помощью `ya dump <command> --help`. Следующие опции поддерживаются большинством команд `ya dump` и влияют на генерируемый сборочный граф:

* `--force-build-depends` (или `-t`, или `-A`) – показывает зависимости 
* `--ignore-recurses` – не обходить RECURSE зависимости совсем;
* `--no-tools` – исключая зависимости для сборки инструментов/утилит сборки (e.g. protoc, yasm etc);

Многие опции `ya make` тоже поддерживаются всеми командами, например:
* `-xx` – перестроить граф с нуля (не использовать кэш графа);
* `-d`, `-r` – граф для отладочного или релизного построения;
* `-k` – игнорировать ошибки кофигурации.

### ya dump modules {#modules}

Показывает список всех зависимостей для цели *target* (текущий каталог, если не задана явно).

Команда: `ya dump modules [option]... [target]...`

**Пример:**
```
spreis@starship:~/ws/arcadia$ ./ya dump modules devtools/ymake | grep sky
module: Library devtools/ya/yalibrary/yandex/skynet $B/devtools/ya/yalibrary/yandex/skynet/libpyyalibrary-yandex-skynet.a
module: Library infra/skyboned/api $B/infra/skyboned/api/libpyinfra-skyboned-api.a
module: Library skynet/kernel $B/skynet/kernel/libpyskynet-kernel.a
module: Library skynet/api/copier $B/skynet/api/copier/libpyskynet-api-copier.a
module: Library skynet/api $B/skynet/api/libpyskynet-api.a
module: Library skynet/api/heartbeat $B/skynet/api/heartbeat/libpyskynet-api-heartbeat.a
module: Library skynet/library $B/skynet/library/libpyskynet-library.a
module: Library skynet/api/logger $B/skynet/api/logger/libpyskynet-api-logger.a
module: Library skynet/api/skycore $B/skynet/api/skycore/libpyskynet-api-skycore.a
module: Library skynet/api/srvmngr $B/skynet/api/srvmngr/libpyskynet-api-srvmngr.a
module: Library skynet/library/sky/hostresolver $B/skynet/library/sky/hostresolver/libpylibrary-sky-hostresolver.a
module: Library skynet/api/conductor $B/skynet/api/conductor/libpyskynet-api-conductor.a
module: Library skynet/api/gencfg $B/skynet/api/gencfg/libpyskynet-api-gencfg.a
module: Library skynet/api/hq $B/skynet/api/hq/libpyskynet-api-hq.a
module: Library skynet/api/netmon $B/skynet/api/netmon/libpyskynet-api-netmon.a
module: Library skynet/api/qloud_dns $B/skynet/api/qloud_dns/libpyskynet-api-qloud_dns.a
module: Library skynet/api/samogon $B/skynet/api/samogon/libpyskynet-api-samogon.a
module: Library skynet/api/walle $B/skynet/api/walle/libpyskynet-api-walle.a
module: Library skynet/api/yp $B/skynet/api/yp/libpyskynet-api-yp.a
module: Library skynet/library/auth $B/skynet/library/auth/libpyskynet-library-auth.a
module: Library skynet/api/config $B/skynet/api/config/libpyskynet-api-config.a
```

### ya dump relation {#relation}

Находит и показывает зависимость между текущим модулем (текущий каталог) и *target*-модулем.

Команда: `ya dump relation [option]... [target]...`

**Пример:**
```
~/arcadia/devtools/ymake/bin$ ya dump relations contrib/libs/libiconv
Directory (Start): $S/devtools/ymake/bin ->
Program (Include): $B/devtools/ymake/bin/ymake ->
Library (BuildFrom): $B/devtools/ymake/libdevtools-ymake.a ->
Library (Include): $B/devtools/ymake/lang/libdevtools-ymake-lang.a ->
Library (Include): $B/library/charset/liblibrary-charset.a ->
Directory (Include): $S/contrib/libs/libiconv
```

{% note info %}

* Граф зависимостей строится для проекта в текущей директории. Это можно поменять опцией `-С`, опция `--from` только выбирает стартовую точку в этом графе.
* `target` - имя модуля как пишет `ya dump modules` или директория проекта этого модуля. 

{% endnote %}


{% note warning %}

Между модулями путей в графе зависимостей может быть несколько. Утилита находит и показывает один из них (произвольный).

{% endnote %}

Есть ключ `-t`, который при вычислении зависимостей учитывает ещё и зависимости тестов (`DEPENDS` и `RECURSE_FOR_TESTS`).


### ya dump all-relations {#all-relations}

Выводит в формате dot все зависимости во внутреннем графе между *source* (по умолчанию – всеми целями из текущего каталога) и *target*.

Команда: `ya dump all-relations [option]... [--from <source>] --to <target>`

{% note info %}

* Граф зависимостей строится для проекта в текущей директории. Это можно поменять опцией `-С`, опция `--from` только выбирает стартовую точку в этом графе.
* `target` - имя модуля как пишет `ya dump modules` или директория проекта этого модуля. 

{% endnote %}

**Пример:**
```
~/arcadia/devtools/ymake/bin$ ya dump all-relations --to contrib/libs/libiconv | dot -Tpng > graph.png
``` 
![graph](../assets/all-relations-graph1.png "Граф зависимостей" =583x536)

С помощью опции `--from` можно поменять начальную цель:
```
~/arcadia/devtools/ymake/bin$ ya dump all-relations --from library/xml/document --to contrib/libs/libiconv
``` 
![graph](../assets/all-relations-graph2.png "Граф зависимостей" =312x327)

Опции `--from` и `--to` можно указывать по несколько раз. Так можно посмотреть на фрагмент внутреннего графа, а не рисовать его целиком с `ya dump dot-graph`.


### ya dump dot-graph {#dot-graph}

Выводит в формате dot все зависимости данного проекта. Это аналог `ya dump modules` c нарисованными зависимости между модулями.

Команда: `ya dump dot-graph [OPTION]... [TARGET]...`


### ya dump dep-graph/json-dep-graph {#dep-graph}

Выводит во внутреннем формате (с отступами) или в форматированный JSON граф зависимостей ymake.

Команда:
`ya dump dep-graph [OPTION]... [TARGET]...`
`ya dump json-dep-graph [OPTION]... [TARGET]...`

### ya dump build-plan {#build-plan}

Выводит в форматированный JSON граф сборочных команд примерно соответствующий тому, что будет исполняться при запуске команды [ya make](https://wiki.yandex-team.ru/yatool/make/).
Более точный граф можно получить запустив `ya make -j0 -k -G`

Команда:
`ya dump build-plan [OPTION]... [TARGET]...`

{% note warning %}

Многие опции фильтрации не имеют смысла для графа сборочных команд и тут не поддерживаются.

{% endnote %}

### ya dump loops/peerdir-loops {#loops}

Выводит циклы по зависимостям между файлами или проектами. ya dump peerdir-loops - только зависимости по PEERDIR между проектами, ya dump loops - любые зависимости включая циклы по инклудам между хедерами.

{% note alert %}

Циклы по PEERDIR в Аркадии запрещены и в здоровом репозитории их быть не должно.

{% endnote %}

Команда:
`ya dump loops [OPTION]... [TARGET]...`
`ya dump peerdir-loops [OPTION]... [TARGET]...`

### ya dump compile-commands/compilation-database {#compile}

Выводит через запятую список JSON-описаний сборочных команд. Каждая команда состоит из 3х свойств: `"command"`, `"directory"`, `"file"`.

Команда:
`ya dump compile-commands [OPTION]... [TARGET]...`

Опции:
* `-q, --quiet` - ничего не выводить. Можно использовать для селективного чекаута в svn.
* `--files-in=FILE_PREFIXES` - выдать только команды с подходящими префиксом у `"file"`
* `--files-in-targets=PREFIX` - фильтровать по префиксу `"directory"`
* `--no-generated` - исключить команды обработки генерированных файлов
* `--cmd-build-root=CMD_BUILD_ROOT` - использьзовать путь как сборочную директорию в командах
* `--cmd-extra-args=CMD_EXTRA_ARGS` - добавить опции в команды
* Поддержано большинство сборочных опций [`ya make`](./ya_make/index.md)

**Пример:**
```
~/ws/arcadia$ ya dump compilation-database devtools/ymake/bin
...
{
    "command": "clang++ --target=x86_64-linux-gnu --sysroot=/home/spreis/.ya/tools/v4/244387436 -B/home/spreis/.ya/tools/v4/244387436/usr/bin -c -o /home/spreis/ws/arcadia/library/cpp/json/fast_sax/parser.rl6.cpp.o /home/spreis/ws/arcadia/library/cpp/json/fast_sax/parser.rl6.cpp -I/home/spreis/ws/arcadia -I/home/spreis/ws/arcadia -I/home/spreis/ws/arcadia/contrib/libs/linux-headers -I/home/spreis/ws/arcadia/contrib/libs/linux-headers/_nf -I/home/spreis/ws/arcadia/contrib/libs/cxxsupp/libcxx/include -I/home/spreis/ws/arcadia/contrib/libs/cxxsupp/libcxxrt -I/home/spreis/ws/arcadia/contrib/libs/zlib/include -I/home/spreis/ws/arcadia/contrib/libs/double-conversion/include -I/home/spreis/ws/arcadia/contrib/libs/libc_compat/include/uchar -fdebug-prefix-map=/home/spreis/ws/arcadia=/-B -Xclang -fdebug-compilation-dir -Xclang /tmp -pipe -m64 -g -ggnu-pubnames -fexceptions -fstack-protector -fuse-init-array -faligned-allocation -W -Wall -Wno-parentheses -Werror -DFAKEID=5020880 -DARCADIA_ROOT=/home/spreis/ws/arcadia -DARCADIA_BUILD_ROOT=/home/spreis/ws/arcadia -D_THREAD_SAFE -D_PTHREADS -D_REENTRANT -D_LIBCPP_ENABLE_CXX17_REMOVED_FEATURES -D_LARGEFILE_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -UNDEBUG -D__LONG_LONG_SUPPORTED -DSSE_ENABLED=1 -DSSE3_ENABLED=1 -DSSSE3_ENABLED=1 -DSSE41_ENABLED=1 -DSSE42_ENABLED=1 -DPOPCNT_ENABLED=1 -DCX16_ENABLED=1 -D_libunwind_ -nostdinc++ -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mcx16 -std=c++17 -Woverloaded-virtual -Wno-invalid-offsetof -Wno-attributes -Wno-dynamic-exception-spec -Wno-register -Wimport-preprocessor-directive-pedantic -Wno-c++17-extensions -Wno-exceptions -Wno-inconsistent-missing-override -Wno-undefined-var-template -Wno-return-std-move -nostdinc++",
    "directory": "/home/spreis/ws/arcadia",
    "file": "/home/spreis/ws/arcadia/library/cpp/json/fast_sax/parser.rl6.cpp"
},
...
```

### ya dump groups {#groups}

Выводит в виде JSON информацию о всех или выбранных группах (имя, участников, почтовый список рассылки)

Команда: `ya dump groups [OPTION]... [GROUPS]...`

Опции:
* `--all_groups` - вся информация о группах (default)
* `--groups_with_users` - только информация об участниках
* `--mailing_lists` - только списки рассылки

### ya dump json-test-list – информация о тестах {#test-list}

Выводит форматированный JSON с информацией о тестах 

Команда: `ya dump json-test-list [OPTION]... [TARGET]...`

Опции:
* `--skip-deps` - только тесты заданного проекта, без тестов зависимых
* `--help` - список всех опций

### ya dump recipes – информация о поднимаемых рецептах {#recipes}

Выводит форматированный JSON с информацией о рецептах (тестовых окружениях), поднимаемых в тестах

Команда: `ya dump recipes [OPTION]... [TARGET]...`

Опции:
* `--json` - выдать информацию о рецептах в формате JSON
* `--skip-deps` - только рецепты заданного проекта, без рецептов зависимых
* `--help` - список всех опций

### ya dump conf-docs {#docs}

Генерирует документацию по модулям и макросам в формате Markdown (для чтения) или JSON (для автоматической обработки).

Документация автоматически генерируется и коммитится раз в день в Аркадию по адресам:

* [arcadia/build/docs/readme.md](https://a.yandex-team.ru/arc/trunk/arcadia/build/docs/readme.md) - публичные макросы и модули
* [arcadia/build/docs/all.md](https://a.yandex-team.ru/arc/trunk/arcadia/build/docs/all.md) - все макросы и модули

Команда: `ya dump conf-docs [OPTIONS]... [TARGET]...`

Опции:
* `--dump-all` - включая внутренние модули и макросы, которые нельзя использовать в `ya.make`
* `--json` - информация обо всех модулях и макросах, включая внутренние, в формате JSON


### ya dump debug {#debug}
Собирает отладочную информацию о последнем запуске `ya make` и загружает её на sandbox

Команда: `ya dump debug [last|N] [OPTION]`

`ya dump debug` — посмотреть все доступные для загрузки bundle   
`ya dump debug last --dry-run` — cобрать bundle от последнего запуска `ya make`, но не загружать его на sandbox   
`ya dump debug 2` — cобрать **пред**последний bundle и загрузить его на sandbox    
`ya dump debug 1` — cобрать последний bundle и загрузить его на sandbox    

Опции:
* `--dry-run` — собрать отладочный bundle от последнего запуска, но не загружать его на sandbox

**Пример:**
```
┬─[v-korovin@v-korovin-osx:~/a/arcadia]─[11:50:28]
╰─>$ ./ya dump debug

10: `ya-bin make -r /Users/v-korovin/arc/arcadia/devtools/ya/bin -o /Users/v-korovin/arc/build/ya --use-clonefile`: 2021-06-17 20:16:24 (v1)
9: `ya-bin make devtools/dummy_arcadia/hello_world/ --stat`: 2021-06-17 20:17:06 (v1)
8: `ya-bin make -r /Users/v-korovin/arc/arcadia/devtools/ya/bin -o /Users/v-korovin/arc/build/ya --use-clonefile`: 2021-06-17 20:17:32 (v1)
7: `ya-bin make devtools/dummy_arcadia/hello_world/ --stat`: 2021-06-17 20:18:14 (v1)
6: `ya-bin make -r /Users/v-korovin/arc/arcadia/devtools/ya/bin -o /Users/v-korovin/arc/build/ya --use-clonefile`: 2021-06-18 12:28:15 (v1)
5: `ya-bin make -r /Users/v-korovin/arc/arcadia/devtools/ya/test/programs/test_tool/bin -o /Users/v-korovin/arc/build/ya --use-clonefile`: 2021-06-18 12:35:17 (v1)
4: `ya-bin make -A devtools/ya/yalibrary/ggaas/tests/test_like_autocheck -F test_subtract.py::test_subtract_full[linux-full]`: 2021-06-18 12:51:51 (v1)
3: `ya-bin make -A devtools/ya/yalibrary/ggaas/tests/test_like_autocheck -F test_subtract.py::test_subtract_full[linux-full]`: 2021-06-18 13:04:08 (v1)
2: `ya-bin make -r /Users/v-korovin/arc/arcadia/devtools/ya/bin -o /Users/v-korovin/arc/build/ya --use-clonefile`: 2021-06-21 10:26:31 (v1)
1: `ya-bin make -r /Users/v-korovin/arc/arcadia/devtools/ya/test/programs/test_tool/bin -o /Users/v-korovin/arc/build/ya --use-clonefile`: 2021-06-21 10:36:21 (v1)
```


### ya dump --help  {#help}

Выводит список всех доступных команд - есть ещё ряд команд кроме описанных выше, но большинство из них используются внутри сборочной инфраструктуры и мало интересны. 
Про каждую можно спросить `--help` отдельно.

Команда: `ya dump --help`

