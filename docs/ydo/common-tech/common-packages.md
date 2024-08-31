# Общие пакеты

В ydo используются следующие общие пакеты:
* [@yandex-int/ydo-types](https://a.yandex-team.ru/arc_vcs/frontend/packages/ydo-types)
* [@yandex-int/ydo-resource-schema](https://a.yandex-team.ru/arc_vcs/frontend/packages/ydo-resource-scheme)
* [@yandex-int/ydo-linter-configs](https://a.yandex-team.ru/arc_vcs/frontend/packages/ydo-linter-configs)

### @yandex-int/ydo-types

Набор общих тайпингов

### @yandex-int/ydo-resource-schema

Набор схем для валидации ресурсов

## @yandex-int/ydo-linter-configs

Пакет с конфигами линтеров ydo для eslint и stylelint

## Разработка

Для разработки и локального тестирования общих пакетов можно воспользоваться следующими командами:
* в ydo выполнить команду `npm link <package_name>`, после внесения изменения необохдимо только пересобирать пакет, без линковки
* в корне монорепы выполнить команду `npx lerna bootstrap --scope=@yandex-int/ydo --scope=<package_name>`
