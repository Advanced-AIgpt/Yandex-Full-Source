# Фантастические srcrwr и как их использовать
В стеке Алисы есть множество способов использовать srcrwr, они появлялись в разное время, используют разные технологии и имеют немного разные форматы и ограничения. В начале описаны самые распространенные варианты использования, ниже расписана логика srcrwr на различных бэкендах


## Частые варианты использования
Большинство из кейсов, описанных ниже, можно объединять

### Я хочу поменять какую-либо ноду в графе сценария или графе мегамайнда {#srcrwr-one-node}
`srcrwr=NODE_NAME:host:port` или `srcrwr=NODE_NAME:scheme://host:port/path` или `srcrwr=NODE_NAME:host:port:timeout`. В последнем случае переопределяется таймаут, по умолчанию там пишутся миллисекунды, но можно добавить суффикс `s`, чтобы получить секунды.
  * Пример: меняем ноду MUSIC_SCENARIO_RENDER в графе [music_run](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/music_run.json?rev=r8240521#L109) `srcrwr=MUSIC_SCENARIO_RENDER:my_host.yandex.net:12346`
  * Пример: меняем ноду MUSIC_SCENARIO_RENDER в графе [music_run](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/music_run.json?rev=r8240521#L109) на кастомную с таймаутом 100 секунд `srcrwr=MUSIC_SCENARIO_RENDER:my_host.yandex.net:12346:100000`

Таким же образом можно поменять ноды источников в [графе мегамайнда](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/megamind.json?rev=r8240252)
  * Пример: `srcrwr=SKILL_PROACTIVITY_HTTP:my_host.yandex.net:12346`

Чтобы поменять две ноды, надо добавить несколько srcrwr.

### Я хочу поменять мегамайнд {#srcrwr-megamind-nodes}
`srcrwr=MEGAMIND_ALIAS:host:port`, порт нужно брать grpc, в проде и других инсталляциях в Няне -- 83, при локальном запуске он пишется в логе и обычно равен `порт запуска +3`, этот кейс аналогичен [Я хочу поменять несколько нод моего AppHostPure сценария](#srcrwr-pure-scenario-run)
  * Пример: `srcrwr=MEGAMIND_ALIAS:my_host.yandex.net:17893`

### Я хочу поменять бегемот {#srcrwr-begemot-nodes}
`srcrwr=ALICE__BEGEMOT_WORKER_MEGAMIND:host:port`, порт нужно брать grpc, в проде и других инсталляциях в Няне -- 81, при локальном запуске он пишется в логе.
  * Пример: `srcrwr=ALICE__BEGEMOT_WORKER_MEGAMIND:my_host.yandex.net:81`

[Подробнее о бетах бегемота](https://wiki.yandex-team.ru/alice/begemot/beta/).

[О том, как развернуть локально](https://wiki.yandex-team.ru/begemot/#kakzapustit) -- рекомендуется использовать ya make способ, в качестве шарда требуется указывать `SHARD=$ARCADIA/search/begemot/data/Megamind/`

### Я хочу использовать локально поднятый аппхост для run запросов в сценарии {#srcrwr-apphost}
[^Скрипт для запуска аппхоста с локальными графами^](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/http_adapter/localhost/run-prod.sh?rev=r8197042)
Просто выставляем dev машину в качестве VinsUrl, в этом случае, будут браться те графы, которые вы запустили. Все запросы в Мегамайнд, источники и run стадии сцнариев будут выполняться с вашей машины.
  * Пример: `http://my_host.yandex.net:40004/speechkit/app/pa/`

### Я хочу поменять запрос в AppHostProxy сценарий, но только Run стадию {#srcrwr-proxy-scenario-run}
Полностью соответствует пункту про [изменение одной ноды в графе](#srcrwr-one-node)
`srcrwr=SCENARIO_DIALOGOVO_RUN:host:port[:timeout]` или `srcrwr=SCENARIO_DIALOGOVO_RUN:scheme://host:port/path` (слэш в конце пути не нужен). Такой srcrwr поменяет [эту ноду](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/megamind_scenarios_run_stage.json?rev=r8252865#L233)
NB: нельзя объединять с [изменением всех походов в AppHostProxy сценарии](#srcrwr-proxy-scenario)
  * Пример: `srcrwr=SCENARIO_IOT_RUN:iot-dev.quasar.yandex.net:80`
  * Пример: 
  ```
  srcrwr=SCENARIO_DIALOGOVO_RUN:http://paskills-common-testing.alice.yandex.net:80/dialogovo-priemka/megamind
  ```
Для Continue, Apply и Commit выполняется аналогично, только `RUN` заменяется на соответствующую стадию:
  * Пример: `srcrwr=SCENARIO_IOT_CONTINUE:iot-dev.quasar.yandex.net:80`
  * Пример: `srcrwr=SCENARIO_DIALOGOVO_CONTINUE:http://paskills-common-testing.alice.yandex.net:80/dialogovo-priemka/megamind`

### Я хочу поменять запрос в AppHostProxy сценарий {#srcrwr-proxy-scenario}
`srcrwr=Dialogovo:host:port` или `srcrwr=Dialogovo:scheme://host:port/path` (слэш в конце пути не нужен).
Так можно перенаправить и те запросы, которые выполняются через apphost (run), и те, что посылаются самим Мегамайндом по http (continue, apply, commit)
NB: нельзя объединять с [изменением только Run стадии](#srcrwr-proxy-scenario-run)
  * IoT: 
  ```
  srcrwr=IoT:iot-dev.quasar.yandex.net:80
  ```
  * Dialogovo:
  ```
  srcrwr=Dialogovo:http://paskills-common-testing.alice.yandex.net:80/dialogovo-priemka/megamind
  ```
  * Vins:
  ```
  http://vins.alice.yandex.net/speechkit/app/pa/?srcrwr=Vins:http://zubchick-dev.sas.yp-c.yandex.net:8000/proto/app/pa
  ```


### Я хочу поменять несколько нод моего AppHostPure сценария {#srcrwr-pure-scenario-run}
Есть два варианта: поменять каждую ноду, как описано [в первом кейсе](#srcrwr-one-node), или использовать алиасы. Например, для нод сценария музыки есть алиас [MUSIC_SCENARIO](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/music_run.json?rev=r8240521#L82), если его переопределить, то перепределятся все ноды, которые имеют такой алиас, в частности, в этом графе переопределятся ноды MUSIC_SCENARIO_PREPARE, MUSIC_SCENARIO_RENDER, MUSIC_SCENARIO_WEEKLY_PROMO_PREPARE, PACK_HTTP_RUN_RESPONSE, UNPACK_HTTP_RUN_REQUEST_INTERNAL, а в других графах ноды, которые имеют такой же алиас
  * Пример: `srcrwr=MUSIC_SCENARIO_RENDER:my_host.yandex.net:12346` поменяет только ноду MUSIC_SCENARIO_RENDER
  * Пример: `srcrwr=MUSIC_SCENARIO:my_host.yandex.net:12346` поменяет все ноды, у которых есть алиас MUSIC_SCENARIO

### Я хочу поменять Басс, в который ходит Винс {#srcrwr-vins-bass}
Описано [тут](#internal-logic-vins)
  * Пример: `srcrwr=BASS:my_host.yandex.net:7777`


## Внутренняя логика
Ниже описаны подробности обработки srcrwr на разных бэкендах

### AppHost {#internal-logic-apphost}
Описано в [документации Аппхоста](https://docs.yandex-team.ru/apphost/pages/cgi#srcrwr). Там еще описан вариант с изменение бэкенда, но это требует знания, какие ноды во всем стеке его используют. Таким образом, например, реализована приемка Common шарда Голливуда: меняются все ноды, которые ходят в бэкенд `HOLLYWOOD_COMMON`. Это единственный вариант, который умеет изменять таймаут, во всех остальных реализациях таймаут может либо не распарситься, тогда srcrwr не применится, либо привести к неправильному походу в источник.

### Vins {#internal-logic-vins}
Тоже постепенно вымирает, но медленнее. Можно менять Bass `srcrwr=BASS:host:port`

### Bass (Vins) {#internal-logic-bass-vins}
`srcrwr=BASS_SourceName:scheme://host:port/path`, здесь надо поменять SourceName, префикс `BASS_` остается на месте. [Список источников](https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/configs/production_config.json?rev=r8168303#L131)

### Bass (Hollywood) {#internal-logic-bass-hollywood}
`srcrwr=BASS_SRCRWR_SourceName:scheme://host:port/path`, здесь надо поменять SourceName, префикс `BASS_SRCRWR_` остается на месте. [Список источников](https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/configs/production_config.json?rev=r8168303#L131)
