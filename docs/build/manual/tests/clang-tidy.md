## clang tidy {#clang_tidy}
Для C++ поддержан линтинг исходного кода, из `LIBRARY`, `PROGRAM`, `DLL`, `G_BENCHMARK`, `UNITTEST`, `GTEST` с помощью `clang-tidy`.
Для запуска линтинга нужно вызвать `ya make -t -DTIDY`.

{% note info %}

При запуске `ya make` с флагом `-DTIDY`, вместо узлов компиляции запускается `clang-tidy` для соответствующих исходников.
В этом режиме сборки не происходит, так как граф строится в другой конфигурации, хотя и генерируются все необходимые зависимости для тайдинга.
Поэтому `clang-tidy` тесты, и только они могут быть запущены при указании `-DTIDY`.

{% endnote %}

По умолчанию `clang-tidy` использует базовый аркадийный [конфиг](https://a.yandex-team.ru/arc_vcs/build/config/tests/clang_tidy/config.yaml).
Изменения в этом конфиге должны быть согласованы с [cpp-com@](/devtools/rules/intro#cpp-committee).

Чтобы подключить свой проект к линтингу в автосборке, укажите нужный путь в этом [файле](https://a.yandex-team.ru/arc_vcs/autocheck/linux/clang_tidy_targets.inc#L2).

Некоторые ошибки, обнаруженные с помощью проверок в `clang-tidy`, могут быть автоматически исправлены.
В этом случае в сниппете `suite` будет написана команда для их автоматического исправления с помощью `ya clang-tidy --fix`.

### Кастомные конфиги для clang-tidy {#tidy_project}
Если ваш проект уже подключен к тайдингу в автосборке и вам хочется расширить
множество проверок, вы можете воспользоваться механизмом проектных конфигов для
`clang-tidy`

**Немного терминологии**

- *Базовый конфиг* - Конфиг, описывающий основные проверки и их опции для проверок `clang-tidy`. Конфиг декларируется [тут](https://a.yandex-team.ru/arc/trunk/arcadia/build/yandex_specific/config/clang_tidy/tidy_default_map.json). При создании нового базового конфига, туда заносятся только те проверки и опции, который конфликтуют с `arcadia style guide`.
**Если ваш проект использует аркадийный стиль, вам не нужно создавать свой базовый конфиг!**
- *Базовый общеаркадийный конфиг* - [конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/build/config/tests/clang_tidy/config.yaml), по умолчанию являющийся базовым. в этом конфиге описываются основные опции и проверки из `arcadia style guide`.
- *Проектный конфиг* - конфиг, относящийся к отдельному проекту. Создается авторами проектов, декларируется [тут](https://a.yandex-team.ru/arc/trunk/arcadia/build/yandex_specific/config/clang_tidy/tidy_project_map.json). Этот конфиг будет объединен с базовым конфигом, который используется в вашем проекте.


#### Ограничения, накладываемые на проектный конфиг {#tidy_project_restrictions}
- Конфиг не должен противоречить `arcadia-style-guide`.
- Проверки из конфига не должны сильно замедлять тайдинг.

Для выполнения этих требований мы завели [whitelist](https://a.yandex-team.ru/arc/trunk/arcadia/build/tests/config/clang_tidy/tidy_config_validation.py#L4) доступных проверок и их опций. Проверки, не перечисленные в `whitelist`, будут удалены из конфига.
Любые изменения `whitelist` должны быть одобрены [cpp-com@](/devtools/rules/intro#cpp-committee).

#### Создание своего конфига {#new_tidy_project}
`clang-tidy` конфиг - это `yaml` файл, в котором перечислены проверки и их опции.
Список доступных проверок можно найти
[тут](https://clang.llvm.org/extra/clang-tidy/checks/list.html).
Список опций для проверки можно найти на странице проверки.
[пример](https://a.yandex-team.ru/arc/trunk/arcadia/build/config/tests/clang_tidy/config.yaml) готового конфига.

#### Подключение проектного конфига к автосборке {#autocheck_tidy_project}
Чтобы в автосборке начал работать ваш конфиг, достаточно добавить ваш проект в [словарь](https://a.yandex-team.ru/arc/trunk/arcadia/build/yandex_specific/config/clang_tidy/tidy_project_map.json).
В этом словаре ключ - это префикс относительно корня аркадии, к которому будет применяться ваш
конфиг, а значение - путь до конфига относительно корня аркадии.

### Конфиги для проектов, не соответствующих аркадийному style-guide {#tidy_project_style}
Если ваш проект использует стиль, отличный от аркадийного, вам нужно создать свой базовый конфиг.
Чтобы задекларировать ваш базовый конфиг в системе сборки, вы должны добавить этот конфиг в [словарь](https://a.yandex-team.ru/arc/trunk/arcadia/build/yandex_specific/config/clang_tidy/tidy_default_map.json), по аналогии с проектным конфигом.

1) В новом базовом конфиге должны содержаться только проверки и опции, которые конфликтуют с `arcadia-style-guide`.
2) Остальная часть конфига должна быть вынесена в ваш проектный конфиг.
3) Создание и изменение вашего базового конфига должно быть согласовано с [cpp-com@](/devtools/rules/intro#cpp-committee)

#### С каким конфигом будет запускаться проверка? {#tidy_merged_config}
Итоговый конфиг будет сконструирован из вашего проектного конфига и базового конфига. Вот как это будет происходить:
1) Отфильтруются все проверки и опции из проектного конфига, в соответствии с `whitelist`.
2) В итоговый конфиг добавятся оставшиеся проверки из проектного конфига.
3) В итоговый конфиг добавятся проверки из базового конфига.
4) В итоговый конфиг добавятся все опции из базового и проектного конфигов, при возникновении конфликтов побеждает проектный конфиг.

![схема получения конфига](https://wiki.yandex-team.ru/users/iaz1607/tidy-project-announcement/.files/tidy-config-scheme.png)

Из-за фильтрации проектного конфига и дальнейшего объединения с базовым,
может быть непонятно, с каким конфигом запускается `clang-tidy` в автосборке.
Чтобы решить эту проблему, в хендлере `ya clang-tidy` поддержана опция `--show-autocheck-config`,
которая выведет финальный автосборочный конфиг на экран.
#### Локальный запуск с проектным конфигом {#tidy_project_local}
1) `ya make -A -DTIDY` - запустит ваши `clang-tidy` тесты "как в автосборке"
2) `ya clang-tidy --use-autocheck-config` - Вариант, аналогичный предыдущему.
3) `ya clang-tidy --use-autocheck-config --fix` - попытается починить обнаруженные проблемы. Если перед этим запускался вариант (2), то результаты тайдинга возьмутся из кэша.
4) `ya clang-tidy` - запустит проверку с вашим проектным конфигом, игнорируя `whitelist` и базовый конфиг

### Включение tidy в CLion {#clang_tidy_clion}
Для того чтобы CLion подхватил Аркадийный [конфиг clang-tidy](https://a.yandex-team.ru/arc_vcs/build/config/tests/clang_tidy/config.yaml) нужно сгенерировать проект, добавив опцию `--setup-tidy`.

Для того чтобы не нужно было постоянно указывать ключ, его можно указать в [конфиге ya](/yatool/commands/gen_config), добавив секцию вида:
```
[ide.clion]
setup_tidy = true
```
Подробней о настройке CLion проекта можно почитать в документации [ya ide clion](/ya-make/usage/ya_ide/clion).

### Включение tidy в VS Code {#clang_tidy_vscode}
Для того чтобы VS Code подхватил Аркадийный [конфиг clang-tidy](https://a.yandex-team.ru/arc_vcs/build/config/tests/clang_tidy/config.yaml) нужно:
- установить [clang-tidy](https://marketplace.visualstudio.com/items?itemName=notskm.clang-tidy) plugin
- сгенерировать VS Code проект, выполнив `ya ide vscode-clangd --setup-tidy` в директории нужного проекта

Для того чтобы не нужно было постоянно указывать ключ, его можно указать в [конфиге ya](/yatool/commands/gen_config), добавив секцию вида:
```
[ide.vscode-clangd]
setup_tidy = true
```
Подробней о настройке VS Code проекта можно почитать в документации [ya ide vscode-clangd](/ya-make/usage/ya_ide/vscode).
