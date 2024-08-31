## Визуализация ответа Алисы

Парсит json ответ ue2e прокачек или логов Алисы, формирует человекочитаемые описания для:
* Сценария generic_scenario
* Действия Алисы action
* Состояния станции device_state

#### Описание файлов:
* `visualize_request.py` — точка входа, публичные методы:
    * `get_request_visualize_data` — возвращает объект со всеми человекочитаемыми полями
    * `get_action_visualize_data` — возвращает челоекочитаемое действие колонки, формируется по директивам запроса
    * `get_device_state_visualize_data` — возвращает челоекочитаемое состояние устройства, формируется по device_state
* `visualize_directive.py` — визуализация директивы в поле `action`, действия, которое сделала Алиса, помимо голосового ответа answer
* `visualize_state.py` — визуализация состояния станции device_state
* `generic_scenario_to_human_readable.py` — человекочитаемые описания для generic_scenario. Файл нужно актуализировать при появлении новых сценариев в момент ue2e приёмки сценария

Дополнительно крупные куски кода визуацизации экшна/стейта по сценариям вынесены в отдельные файлы:
* visualize_alarms_timers.py
* visualize_geo.py
* visualize_iot.py
* visualize_multiroom.py
* visualize_music.py
* visualize_open_uri.py
* visualize_video.py

#### Тесты
При изменении парсинга необходимо добавлять новый тест по инструкции https://a.yandex-team.ru/arc/trunk/arcadia/alice/analytics/operations/priemka/alice_parser/tests/readme.md#kak-napisatь-test-na-visualize_quasar_sessions
