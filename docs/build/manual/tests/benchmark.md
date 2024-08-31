# Бенчмарки

Для написания бенчмарков мы используем библиотеку [google benchmark]
и цель `G_BENCHMARK` в `ya.make` файле — [пример][benchmark example].

`G_BENCHMARK` поддержан на уровне `ya make -t`

Все подключенные к автосборке `G_BENCHMARK` запускаются в CI по релевантным коммитам и накапливают историю метрик, которая может быть полезна для поиска регрессий.

Для `G_BENCHMARK` доступно:

* Листинг бенчмарков: ya m -rAL - выведет список доступных бенчмарков
* Фильтрация: ya m -rAF \<fnmatch expression\> - запустит все бенчмарки, удовлетворяющие \<fnmatch expression\>
* Сбор корок в случае таймаута или падения бенчмарка по сигналу
* Graceful обработка таймаута - при таймаутах мы сохраняем весь текущий прогресс и строим итоговый отчет о тестировании, основываясь на этой информации.
* Сбор метрик для успешно завершившихся бенчмарков. При запуске в автосборке, эти метрики можно будет увидеть на странице теста в CI.

Для `G_BENCHMARK` недоступно:

* `FORK_(SUB)TESTS`: предполагается, что G_BENCHMARK это микробенчмарки и в параллельном запуске нет необходимости.

[Пример бенчмарка в CI][benchmark CI example]

[google benchmark]: https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/benchmark/README.md
[benchmark example]: https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/benchmark
[benchmark CI example]: https://ci.yandex-team.ru/test/6d3918a530fea39ef4b35a57bcebc01c#cpu_time_ns
