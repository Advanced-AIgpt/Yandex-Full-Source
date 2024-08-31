## Test Scenario

Test scenario - наиболее простой и бесполезный сценарий Hollywood.
Он ничего не делает, и возвращает иррелевантный ответ, в котором содержится фраза, произнесенная пользователем.

Зато этот сценарий умеет логгировать все запрошенные данные в лог.
Его можно использовать в локальной отладке, если требуется проверить входные данные от Мегамайнда и/или других источников.

Логгирование производится в стандартный протокол Hollywood на уровне `LOG_DEBUG()`.

## Чтобы воспользоваться test_scenario

1. Включите флаг эксперимента `mm_enable_protocol_scenario=TestScenario`
2. Выберите, какие данные необходимо сохранять в лог: `test_scenario_logging=tag1,tag2,...`, где теги могут принимать следующие значения:
   * meta - логгировать RequestMeta
   * base - логгировать BaseRequestProto целиком (также есть возможность логгировать только части BaseRequestProto - см ниже)
   * interfaces - логгировать BaseRequestProto.TInterfaces
   * memento - логгировать BaseRequestProto.TMementoData
   * devicestate - логгировать BaseRequestProto.TDeviceState
   * также в качестве тегов можно использовать названия любых датасорсов
   Пример: `test_scenario_logging=meta,interfaces,BEGEMOT_EXTERNAL_MARKUP,USER_LOCATION`

Флагов эксперимента можно завести несколько, описывая теги последовательно через запятую, или в отдельных экспах.
В примере ниже сценарий залоггирует мету, интерфейсы и 3 указанных датасорса (при их наличии):

```
test_scenario_logging=meta,interfaces
test_scenario_logging=USER_LOCATION
test_scenario_logging=BLACK_BOX,WEB_SEARCH_DOCS
```

## Важно

1. При локальном запуске Голливуда не забудьте подключить сценарий `TEST_SCENARIO` в сборку:

```
run-hollywood -C=MY_SCENARIO_NAME -C=TEST_SCENARIO
```

2. Добавляя новые датасорсы в запросы Мегамайнда, добавляйте их в конфигурацию тестового сценария (alice/megaming/configs).
