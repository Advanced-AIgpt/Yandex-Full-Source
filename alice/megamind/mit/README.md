# Megamind integration test
Интеграционные тесты, тестирующие megamind как бэкенд и графы апхоста мегамайнда

**Как написать базовый тест**

Легче всего посмотреть как написан другой простой тест https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/basic_run_request


Создать папку в директории `alice/megamind/mit/tests` внутри которого лежат ya.make и my_test.py.

ya.make должен включать:

`INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)` и 

`SIZE(MEDIUM)`

Для построения запроса в мм нужно использовать [MegamindRequestBuidler](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/library/request_builder/request_builder.py), запрос оборачивается в [обертку](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/library/response/wrapper.py), если вам нужно какое-то поле и его еще нет в интерфейсе можете его сами добавить :). Не рекомендуется парсить json'ы в коде текста, делайте это пожалуйста через ResponseWrapper.

После написания теста нужно его канонизировать (даже если он не канонизационный - это необходимо для генерации стабов) командой 

`ya make -Ar -F *my_test_name* -Z -DMIT_GENERATOR`

При канонизации создастся файл eventlog_data.inc в директории теста (его нужно коммитить в аркадию), там сохранены стабы. Прогонять тест нужно стандартной командой `ya make -Ar -F *my_test_name*`

**Как написать продвинутый тест**

Примеры:
* тест запись аналитики https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/postpone_log_writer/postpone_log_writer.py
* тест на механизм прогрева мегамайнда https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/warm_up/warm_up.py
* тест на run+apply (не такой продвинуты но тоже тут) https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/basic_run_apply_request/basic_run_apply_request.py

MIT фреймворк предоставляет возможность mock'ать ноды (например, замокать [сценарий](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/basic_run_apply_request/basic_run_apply_request.py?rev=r9253763#L20), ноду [внутри мегамайнда](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/postpone_log_writer/postpone_log_writer.py?rev=r9253763#L39-42) или любую другую ноду, которою обходит апхост при запросе в мегамайнд).

Чтобы в тесте была возможность мокать, нужно объявить параметр тесте apphost_stubber, в нем интересны 2 метода:
* `mock_node(node_name: str, handler)`, где `node_name` - строка имя ноды, `handler` - функция, которая выполнится вместо кода ноды. `handler` принимет один параметр ctx - апхостовый [контекст](https://a.yandex-team.ru/arc_vcs/apphost/api/service/python/_apphost_api_service.pyx?rev=r9057248#L151). [Пример](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/library/common/util/scenario/responses.py?rev=r9252429#L24-25) handler, который кладет в контекст один айтем (сценарный response).
* `assert_node_input(node_name: str, handler)` - почти то же самое, идейно в `handler`'е стоит смотреть на входной контекст ноды и писать кастомные ассерты, [пример](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/postpone_log_writer/postpone_log_writer.py?rev=r9252429#L44-52).

Отличие от `mock_node` и `assert_node_input` в том что перый выполняется вместо ноды, а второй просто встраивается перед выполнением, поэтому пожалуйста в хэндлерах `assert_node_input` не меняйте контекст, к сожалению на уровне фреймворка это пока нельзя запретить кодом.

Рекомендуется выносить свои константы (имена нод, сценариев и прочие вещи, которые могут быть применены в нескольких тестах) в [alice/megamind/mit/library/common](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/library/common)

**Полезное**

* для удобства можно использовать тестовый [сценарий](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/scenarios/TestScenario.pb.txt), который принимает тестовый [SemanticFrame](https://a.yandex-team.ru/arc_vcs/alice/megamind/protos/common/frame.proto?rev=r9252059#L1632-1637)
* также существует [способ](https://a.yandex-team.ru/arc_vcs/alice/megamind/mit/tests/basic_run_apply_request/basic_run_apply_request.py?rev=r9253763#L15) с помощью флага эксперимента подписать сценарий на какой-либо frame
