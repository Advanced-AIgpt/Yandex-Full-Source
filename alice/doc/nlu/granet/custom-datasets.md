# Работа со своими датасетами

Для работы с произвольным датасетом используется пакет custom-тестов Гранета.
Такие тесты хранятся в [alice/nlu/data/ru/test/granet/custom](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/custom).

В custom-тестах произвольный датасет используется в качестве positive датасета.
При необходимости, можно канонизировать положительные примеры с помощью скрипта `canonize.sh` (генерируется графом внутри пакета).

## Как прогнать Гранет на своём датасете с помощью графа в Нирване {#process-custom-dataset}

Для работы со своим датасетом необходимо собрать сущности для текстов таким [графом](https://nirvana.yandex-team.ru/flow/0a186e3f-39fe-44a6-bf55-3992d3d53032/5f334cf4-bb58-4482-afed-032ed2d3f405/graph) в Нирване.
На вход необходимо подать YT таблицу и указать название датасета, после чего он появится в custom-тестах, останется только указать нужные формы и полученные tsv-файлы в `cases` в `config.json`.
Найти свой PR можно в выходе последнего кубика Commit tar.
Вдобавок, кубик Make batch на выходе выдает статистику часто встречающихся юниграмм, биграмм и триграмм в положительном и отрицательном датасетах для быстрого старта в написании грамматик.
[Пример](https://jing.yandex-team.ru/files/ardulat/Screen%20Shot%202022-01-27%20at%203.26.18%20PM.png) того, как это будет выглядеть.

На вход графу можно так же подать собранный [Top-of-Mind](https://wiki.yandex-team.ru/alice/analytics/newscenariousbasket/tom/) в качестве своего датасета или использовать такой [кубик](https://nirvana.yandex-team.ru/alias/operation/custom-tests-generator) (будет встроен в процесс сбора TOM).
В результате, создастся PR с подготовленным датасетом, конфигом и скриптами.

## Конфиг custom-тестов {#config}

Конфиг custom-тестов выглядит следующим образом:

```json
{
    "language": "ru",
    "cases": [
        {
            "comment": [
                "Для локального тестирования качества.",
                "В выводе нет мусора. Только табличка с замером качества классификатора.",
                "Не запускается в прекомитных тестах."
            ],
            "name": "alarm_cancel",
            "form": "personal_assistant.scenarios.alarm_cancel",
            "positive": "base/alarms_ue2e_dataset.tsv",
            "consider_slots": false,
            "disable_auto_test": true,
            "collect_blockers": true
        },
        {
            "comment": [
                "Для прекомитных тестов.",
                "Датасет target/alarms_cancel.tsv получается канонизацией.",
                "Этот датасет использутся для диффа в ревью.",
                "В нём также можно посмотреть разметку слотов."
            ],
            "name": "alarm_cancel_canonized",
            "form": "personal_assistant.scenarios.alarm_cancel",
            "base": "base/alarms_ue2e_dataset.tsv",
            "positive": "target/alarms_cancel.tsv"
        }
    ]
}
```

В первом тесте конфига из важного описано следующее:
- `"positive": "base/alarms_ue2e_dataset.tsv"` - произвольный датасет, используется в качестве положительных примеров.
- `"consider_slots": false` - флаг в режиме которого не учитывается разметка слотов.
- `"disable_auto_test": true` - флаг в режиме которого отключается запуск пакета в прекоммитных тестах.
- `"collect_blockers": true` - флаг в режиме которого можно посмотреть список блокирующих парсинг фраз.
Результаты этого теста будут записаны в директорию `alice/nlu/data/ru/test/granet/custom/alarms_ue2e_dataset/results/last/alarm_cancel/extra/` в файлы `blockers_all.tsv` и `blockers_aggregated.tsv`.

Во втором тесте конфига датасет используется в качестве base.
Положительные примеры записываются скриптом `canonize.sh` в `target/alarms_cancel.tsv`.
Тест запускается в прекоммитных тестах и учитывает разметку слотов.

Более подробно про параметры конфигов можно почитать [здесь](testing-batch.md#case).

{% note warning %}

Не забудьте добавить свой тестовый пакет в [alice/nlu/data/ru/test/granet/custom/ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/custom/ya.make), чтобы пакет был виден в прекоммитных тестах.
Также пока нужно руками добавить `ya.make` в сам тестовый пакет, это можно сделать по аналогии с соседними датасетами.

{% endnote %}

## Ручной способ прогона датасета {#manual-method}

Демо-скрипт с примером:
[alice/nlu/granet/tools/granet/demo/select/run.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/granet/tools/granet/demo/select/run.sh)	

То же самое можно сделать за один вызов granet, но удобней разделить его на три шага: создание датасета, сбор сущностей, наложение грамматики.
Дело в том, что второй шаг самый долгий и нестабильный, так как каждый сэмпл отправляется по сети в Бегемот.
Поэтому лучше выполнить его один раз, а дальше работать уже со стабами сущностей (стабы пишутся в виде дополнительной колонки в tsv-файле датасета).
