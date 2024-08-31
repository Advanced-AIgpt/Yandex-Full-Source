# Как добавить новое умение

1. Создать папку с названием умения в [alice/protos/endpoint/capabilities](https://a.yandex-team.ru/arcadia/alice/protos/endpoint/capabilities)
2. Создать объект умения с option-ом CapabilityType. Указать CapabilityType в enum-е TCapability.ECapabilityType.
3. Объявить в объекте умения 3 поля - Meta, Parameters, State
   ```
    TCapability.TMeta Meta = 1 [json_name = "meta", (NYT.column_name) = "meta"];
    TParameters Parameters = 2 [json_name = "parameters", (NYT.column_name) = "parameters"];
    TState State = 3 [json_name = "state", (NYT.column_name) = "state"];
    ```
   - Указать в State все, что описывает состояние умения.  
   - Указать в Parameters все, что описывает ограничения на состояние и обрабатываемые директивы 
   - Если State и Parameters состоят из большого количества составных частей, то запчасти для них можно уносить в [alice/protos/data](https://a.yandex-team.ru/arcadia/alice/protos/data), а сами объекты составлять из их агрегатов.
4. Объявить внутри объекта умения набор директив, которые могут быть обработаны устройствами.  
**Важно**: 
   - Разные платформы могут обрабатывать разный набор директив одного и того же умения. Каждая платформа декларирует
   это в поле SupportedDirectives объекта TMeta.
   - Каждая директива должна объявлять опции SpeechKitName и DirectiveType, и 1 обязательное поле Name. SpeechKitName - клиентское название директивы, DirectiveType используется в поле SupportedDirecitves, а Name пока что необходим для целей аналитики. 
   - Для автоматический кодогенерации для директив в Hollywood необходимо добавить include умения в файл `/arcadia/alice/hollywood/library/framework/core/codegen/directives.cpp.jinja2`, добавить директорию умения через PEERDIR и указать сгенерированный прото-файл в OUTPUT_INCLUDES - [пример](https://a.yandex-team.ru/review/2616484/files/14#file-0-186959613:R20).
6. Объявить директивы в oneof-е объекта TDirective, который находится в файле [alice/megamind/protos/scenarios/directives.proto](https://a.yandex-team.ru/arcadia/alice/megamind/protos/scenarios/directives.proto#L53). Это необходимо для того, чтобы они стали известны Megamind.   
7. Объявить внутри объекта умения набор событий, которые умеет генерировать устройство. Как и в случае с директивами, разные платформы могут генерировать разные наборы событий. 
Поддерживаемые события объявляются в поле SupportedEvents объекта TMeta. 
8. Объявить созданное умение в oneof-е объекта TCapabilityHolder, который находится в файле [alice/protos/endpoint/capabilities/all/all.proto](https://a.yandex-team.ru/arcadia/alice/protos/endpoint/capabilities/all/all.proto)
9. Объявить события умения в oneof-е объекта TCapabilityEventHolder, который находится в файле [alice/protos/endpoint/events/all/all.proto](https://a.yandex-team.ru/arcadia/alice/protos/endpoint/events/all/all.proto)
10. Замержить PR и дождаться выкатки бекендов MM/Uniproxy до production. В целях разработки можно воспользоваться trunk-стендами - коммиты на них выкатываются в течение 30-40 минут.  
