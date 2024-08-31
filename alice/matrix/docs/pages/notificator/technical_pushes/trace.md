# Как отследить путь доставки технического пуша по логам

## Логи notificator'а {#notificator}

Отследить технический пуш по логам notificator'а можно при помощи логов в yt, они лежат на hahn в [/logs/matrix-notificator-prod-eventlog](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/matrix-notificator-prod-eventlog).

Для этого надо воспользоваться [вот этой udf](https://sandbox.yandex-team.ru/resource/3276336323/view) для парса framed лога. [Скоро она появится в аркадии.](https://st.yandex-team.ru/ZION-158)

[Пример запроса с этой udf](https://yql.yandex-team.ru/Operations/YsCYLclwvQzimunGJ5kDOePq6djLjwVcBvAj6MTzefs=).
В этом примере указаны все три интересных поля:
1. ```push_id```
2. ```puid```
3. ```device_id```

В каждом из них можно указать ```NULL```, и тогда поиск будет по подмножеству полей (скорее всего вы всегда хотите искать по чему-то одному, все три поля ```NOT NULL``` только для примера).

{% code "./_includes/trace.yql" lang="yql" %}

Скорее всего вы увидите логи двух или трех (зависит от того, справилась ли ```/delivery/push``` послать пуш прям сейчас) ручек:
1. ```/delivery/push``` - ручка, в которой пуш был добавлен в базу, и была попытка сходить в subway прям сейчас
2. ```/devlivery/on_connect``` - ручка, в которую ходит uniproxy, когда девайс подключается, и просит дослать not expired пуши, на которые не приходили ack
3. ```/directive/change_status``` - ручка, в которую приходят ack о доставке пушей

## Логи uniproxy и девайсов {#uniproxy_and_devices}

Если логов notificator'а не хватило, можно посмотреть на логи uniproxy и на логи с девайсов.

К сожалению, официального поддерживаемого метода нет, с некоторой вероятностью может помочь информация из [этого тикета](https://st.yandex-team.ru/VA-2588).
Если информации в тикете не хватило, напишите нам в [чат поддержки](https://docs.yandex-team.ru/alice-matrix/pages/contacts).
