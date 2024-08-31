# Работа с Solomon

Для начала работы следует запросить доступ в Solomon на редактирование для проекта beruapps, может дать [Саша Попсуенко](https://staff.yandex-team.ru/apopsuenko)

## Основные понятия

В соломоне есть два вопроса - куда отсылать и что отсылать.

### Куда отсылать

В соломоне данные (_метрики_) группируются по _сервисам_ и _кластерам_. _Сервис_ и _кластер_ вместе образуют _шард_. Рассмотрим на примере нашего _проекта_ в соломоне:

[Список сервисов](https://solomon.yandex-team.ru/admin/projects/beruapps/services)
* В _сервисах_ настраивается Monitoring Model (как именно solomon будет получать данные - через отправку в него запросов, или же он сам будет куда-то ходить за данными), а также обязательный параметр лейбла присылаемой метрики (может быть любой, просто потом в запросах его обязательно использовать). В данной документации рассматриваем вариант, когда мы сами отправляем метрику в соломон (Monitoring model PUSH)

[Список кластеров](https://solomon.yandex-team.ru/admin/projects/beruapps/clusters)
* В _кластерах_ в принципе ничего настраивать не нужно, если не углубляться.

Далее, допустим мы создали _сервис_ [android_develop_build_success](https://solomon.yandex-team.ru/admin/projects/beruapps/services/beruapps_android_develop_build_success) и используем существующий _кластер_ [UNKNOWN](https://solomon.yandex-team.ru/admin/projects/beruapps/clusters/beruapps_UNKNOWN). В результате мы получаем из данных _кластера_ и _сервиса_ [шард](https://solomon.yandex-team.ru/admin/projects/beruapps/shards/beruapps_UNKNOWN_android_develop_build_success) . Можем посмотреть данные в _шарде_, для этого кликнуть на shard graphs и накликать необходимые метрики. Их много, и некоторые из них уже не пишутся, но суть понятна. Все эти метрики были отправлены на данный _шард_ из сторонних скриптов и теперь они отображаются в соломоне. Про то, как мы их отправляем - далее.

### Что отсылать

У нас есть _шард_, куда мы хотим послать метрику. Он определяет URL нашего запроса, который формируется по принципу: https://solomon.yandex.net/api/v2/push?project=%projectId%&cluster=%cluster%&service=%service%. В нашем примере **projectId**=beruapps, **cluster**=UNKNOWN, **service**=android_develop_build_success. 
В теле запроса же будут как раз значения метрик, которые мы хотим отправить в соломон.

Для начала рассмотрим самый примитивный вариант - отправить просто какую нибудь метрику в формате "название-значение".
Для этого в тело запроса добавляем следующий json: `{"metrics": [{'labels': {"signal": "%metricName%"}, "value": %metricValue%}`.
Здесь: 
* `metrics` - это общий массив всех метрик в нашем запросе, мы можем отправлять сразу несколько различных метрик.
* `labels` - список лейблов для отдельной метрики. Например, мы хотим отправлять информацию о времени сборки для различных веток, и в текущий момент хотим послать информацию о том, что время сборки девелопа составило 1200 секунд. Можно, конечно использовать название метрики уровня "developBuildTime", но гораздо лучше будет разбить название на два лейбла, например: `'labels': {"signal": "buildTime", "branch": "develop"}`. Это нам позволит более гибко строить выборки в соломоне по различным веткам.
* `signal` - это поле из _сервиса_ solomon, "обязательный параметр лейбла присылаемой метрики", про который мы говорили выше. Если его не будет, то метрика не обработается.
* `%metricName%`, `%metricValue%` - название и значение метрики, указываем нужные нам.

Отправляем в нужный нам _шард_ нужные нам данные, не забываем прикрепить [токен авторизации](https://docs.yandex-team.ru/solomon/api-ref/authentication#oauth). Если всё прошло хорошо, то в _шарде_ появятся ваши метрики. Если что-то не получилось, то попробуйте посмотреть [решение проблем](https://docs.yandex-team.ru/solomon/data-collection/troubleshooting).

Это самый базовый вариант отправки метрики, что ещё можно отправлять, следует прочесть в [документации по структурам данных](https://docs.yandex-team.ru/solomon/data-collection/dataformat/json) . Например, в описанном варианте timestamp будет проставляться на основе времени, когда solomon получил ваши данные, но также можно указывать дополнительный параметр "ts", который будет устанавливать нужное нам время отправки метрики.

## Подробная документация

[Документация по Solomon](https://docs.yandex-team.ru/solomon/)

## Примеры

Два скрипта в тимсити, которые выполняют отправку метрик в Solomon. Один запускается каждый час, другой реагирует на окончание сборки в основных тасках тимсити. У них есть по одному шагу, представляющему собой python-скрипт, который собственно собирает нужные метрики и отправляет их в Solomon.

[Ежечасный скрипт](https://teamcity.yandex-team.ru/admin/editRunType.html?id=buildType:Mobile_BlueMarketAndroid_ScheduledMetricsReport&runnerId=RUNNER_5342&cameFromUrl=%2Fadmin%2FeditBuildRunners.html%3Fid%3DbuildType%253AMobile_BlueMarketAndroid_ScheduledMetricsReport%26init%3D1&cameFromTitle=)

[Скрипт по окончании сборки](https://teamcity.yandex-team.ru/admin/editRunType.html?id=buildType:Mobile_BlueMarketAndroid_DevelopBuildReport&runnerId=RUNNER_5342&cameFromUrl=%2Fadmin%2FeditBuildRunners.html%3Fid%3DbuildType%253AMobile_BlueMarketAndroid_DevelopBuildReport%26init%3D1&cameFromTitle=)
