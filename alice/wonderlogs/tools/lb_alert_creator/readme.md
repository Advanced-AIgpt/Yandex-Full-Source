Программа для генерации алертов Logbroker для Solomon.

На вход подаётся конфиг с опциями Logbroker, на выход - json алертов Solomon, которые подходят для создания при помощи REST API.

### Кофинг на вход:
* **account** - аккаунт в Logbroker. Например, [megamind](https://lb.yandex-team.ru/logbroker/accounts/megamind)
* **topic_paths** - пути топиков. Например, [megamind/analytics/log](https://lb.yandex-team.ru/logbroker/accounts/megamind/analytics-log)
* **consumer_paths** - пути потребителей. Например, [shared/hahn-logfeller-shaddow](https://lb.yandex-team.ru/logbroker/accounts/shared/hahn-logfeller-shadow). Можно указывать `shared/hahn-logfeller*`, потому что это подставляется в сенсор Solomon
* **datacenters** - датацентры, в которые происходит запись. Например, `["Man", "Sas", "Vla"]`
* **solomon_project_id** - Solomon project. Например [megamind](https://solomon.yandex-team.ru/admin/projects/megamind)
* **channels** - массив channel из Solomon, куда отправлять алерты. Например, [Juggler](https://solomon.yandex-team.ru/admin/projects/megamind/channels/Juggler) в проекте megamind

### Пример выхода:
```
[
    {
        "channels":[
            {
                "config":{
                    "notifyAboutStatuses":[
                        "ALARM",
                        "NO_DATA",
                        "WARN",
                        "ERROR"
                    ],
                    "repeatDelaySecs":300
                },
                "id":"Juggler"
            },
            {
                "config":{
                    "notifyAboutStatuses":[
                        "ALARM"
                    ],
                    "repeatDelaySecs":1200
                },
                "id":"ran1s_tg"
            }
        ],
        "delaySecs":0,
        "groupByLabels":[
            "host",
            "TopicPath"
        ],
        "id":"logbroker-megamind-messages-written-original",
        "name":"logbroker-megamind-messages-written-original",
        "projectId":"megamind",
        "type":{
            "threshold":{
                "predicateRules":[
                    {
                        "comparison":"LTE",
                        "targetStatus":"ALARM",
                        "threshold":0,
                        "thresholdType":"AVG"
                    }
                ],
                "selectors":"{service='pqproxy_writeSession', OriginDC='*', Topic!='total', Producer!='total', host='Man|Sas|Vla', cluster='lbk', project='kikimr', Account='megamind', TopicPath='megamind/analytics-log|megamind/apphost/prod/access-log|megamind/apphost/prod/error-log|megamind/apphost/prod/event-log|megamind/apphost/prod/profile-log|/megamind/proactivity-log', sensor='MessagesWrittenOriginal'}"
            }
        },
        "windowSecs":300
    }
]
```

С таким описанием можно создать алерт, используя HTTP API Solomon. [Пример](https://docs.yandex-team.ru/solomon/api-ref/rest#post-alert)
