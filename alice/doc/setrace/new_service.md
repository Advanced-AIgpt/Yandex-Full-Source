# SETrace

## У меня новый сервис, хочу, чтобы логи прорастали в SETrace

1. Использовать наш [логгер](https://a.yandex-team.ru/arc/trunk/arcadia/alice/library/logger/logger.h?rev=r9182779#L54)

2. На каждый запрос [создавать его](https://a.yandex-team.ru/arc_vcs/alice/hollywood/library/dispatcher/common_handles/hw_service_handles/hw_service.cpp?rev=r9364164#L33), используя ReqId и RUID из аппхостового контекста

3. В конфиге [TRTLogClient](https://a.yandex-team.ru/arc_vcs/alice/library/logger/logger.h?rev=r9360469#L145) должен быть указан [UnifiedAgentUri](https://a.yandex-team.ru/arc_vcs/alice/library/logger/proto/config.proto?rev=r9360469#L70)

4. Одним из каналов в [unified_agent](https://docs.yandex-team.ru/unified_agent/) должна быть [отправка индексной информации](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/scripts/nanny_files/unified_agent_config_prod.yaml?rev=r9356020#L50) (это пример, в вашем конфиге могут отличаться port'ы плагина alice_rtlog и api, и должны отличаться service_name, topic). Предварительно в Logbroker'е надо создать топик, поддерживаемые кодеки надо выбрать все

5. В [панчере](https://puncher.yandex-team.ru/) создать правило от `_GENCFG_SETRACE_` до сети, в которой развернут ваш сервис, порт надо взять тот, который вы указали в плагине alice_rtlog в конфиге unified_agent'а

6. Выдать роботу @robot-lf-dyn-push права на чтение топика (ReadTopic):
    ```
    ya tool logbroker -s logbroker permissions grant --permissions ReadTopic --subject robot-lf-dyn-push@staff --path <topic_path>
    ```

7. Выдать необходимые права группе megamind:
   ```
   ya tool logbroker -s logbroker permissions grant --permissions ListResources CreateReadRules DescribeResources --subject abc:megamind --path <topic_path>
   ```

8. Кто-нибудь из ответственных за SETrace Алисы должен добавить правило на чтение:
   ```
   ya tool logbroker -s logbroker schema create read-rule --all-original --consumer /alicelogs/prod/rtlog-index/index-reader --topic <topic_path>
   ```
   Также нужно добавить информацию о новом топике в [endpoint'ы](https://yc.yandex-team.ru/folders/foo9qloqe3rckubi9jvu/data-transfer/endpoints) трансфера, надо добавить новый топик в [alicelogs-rtlog-index-topics](https://yc.yandex-team.ru/folders/foo9qloqe3rckubi9jvu/data-transfer/endpoint/dtedvci0hco21ui7n731/view) и переименование таблицы в [alicelogs-rtlog-index](https://yc.yandex-team.ru/folders/foo9qloqe3rckubi9jvu/data-transfer/endpoint/dtejbj2khp8fnaa9ne9a/view)

9.  Profit!!! Логи для запросов за послежние три часа теперь достаются с напрямую с ваших машин. Если вы хотите увеличить это время до 10 дней, то идем дальше

10. Добавить в конфиг unified_agent'а еще один канал для отправки полных логов, [пример](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/scripts/nanny_files/unified_agent_config_prod.yaml?rev=r9356020#L27), у вас будут другие настроки topic и tvm. Предварительно в Logbroker'е надо создать топик, поддерживаемые кодеки надо выбрать все

11. Настроить LogFeller, чтобы данные складывались в YT. [Документация](https://wiki.yandex-team.ru/logfeller/connection/), пример [настройки](https://a.yandex-team.ru/arc_vcs/commit/r9327473). Дождаться выкатки и убедиться, что данные заливаются.

12. Добавить в [настройки SETrace'а](https://a.yandex-team.ru/arc/trunk/arcadia/search/tools/setrace/src/setrace/models/alice.py?rev=r9351937#L67) путь до ваших логов. Дождаться выкатки.

13. Profit!!!
