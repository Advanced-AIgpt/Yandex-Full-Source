# canonize

Инструмент canonize предназначен для облегчения боли и страданий возникающих при работе с it2 тестами в alice/hollywood.
Он позволяет как перегенерировать стаб файлы, так и канонизировать результаты тестов в больших количествах.

Смысл инструмента становится понятен, когда например добавление какого-нибудь поля/хедера/whatever в запрос меняет имена стаб-файлов 
(ответы источников) в 100500 тестах и они все дружно падают. Т.е. кейсы по факту остаются рабочими, но надо запустить на них генератор, 
чтобы он создал новые стабы. Это довольно болезненный процесс. Например надо перегенерить 500 тестов, мы запускаем генератор тестов, 
он отрабатывает для ~450 тестов успешно, а для 50 - нет. Причины могут быть разными, например таймауты/мигания/нестабильность 
хамстерных источников. И надо избирательно рестартануть эти 50 тестов (снова 500 запускать нельзя - долго, да и в результате можен снова 
упасть ~50 тестов, только ДРУГИХ). Canonize призван решать именно такие проблемы.

## Quick start

1) Собираем canonize
```
$ cd alice/hollywood/library/python/testing/canonize
$ ya make
```

2) Первый запуск (например для тестов сценария zero_testing):
```
$ ./canonize <ARCADIA_ROOT>/alice/hollywood/library/scenarios/zero_testing/it2
```
Что при этом происходит. Запускается it2 runner на всех тестах (или на их части, если передать в canonize параметр фильтра -F)
имена упавших тестов запоминаются. Далее запускается it2 generator ТОЛЬКО для упавших тестов.

3) Второй и последующие запуски:
```
$ ./canonize <ARCADIA_ROOT>/alice/hollywood/library/scenarios/zero_testing/it2 --continue
```
Запускается it2 generator для упавших (на предыдущем запуске генератора) тестов.

## Логи

Canonize пишет инфу о том, что он делает в stdout. При этом, он запускает it2 тесты, чей stderr сохраняется в текущей директории:

* canonize_test_run.log - выхлоп от последнего запуска it2 ранера/генератора
* canonize_test_run.log.1 - выхлоп от предыдущего запуска it2 ранера/генератора
* canonize_failed_tests.log - файл, куда canonize записывает имена упавших (на последнем запуске) тестов