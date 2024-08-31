# Общие сведения

## Введение

Алиса живет внутри большого количества разных устройств, каждое из которых имеет свои возможности и ограничения. 
Кроме того, она умеет управлять другими умными устройствами в своем окружении - например, работать в тандеме с модулем, включать лампочки,
управлять розетками, получать данные от датчиков климата или движения. 

**Capabilities** - протокол, с помощью которого можно декларировать умения клиентского устройства, получать связанные с 
ними состояния, описывать ограничения на взаимодействие и реагировать на генерируемые умением события.     

## Предпосылки появления
Раньше устройства описывались с помощью следующих функций:
- Массива строк TSupportedFeatures, являющихся лейблами того, что умеет устройство
- Объекта TDeviceState, описывающего полное состояние устройства
- Большого oneof-а директив в объекте TDirective, который описывает все возможные директивы для всех устройств

Протокол умений объединяет все эти функции в набор независимых объектов, каждый из которых полностью описывает
свою часть состояния устройства и строго ограничивает методы взаимодействия с ним. 

## Терминология 

#### Endpoint

Клиентское устройство. Состоит из уникального идентификатора, мета-информации, текущего статуса (онлайн/оффлайн) и набора независимых умений. 
Находится в файле [endpoint.proto](https://a.yandex-team.ru/arc_vcs/alice/protos/endpoint/endpoint.proto). 
Протокол не разделяет speechkit-based устройства(колонки/телевизоры) и ведомые устройства (датчики/лампочки).   

#### Capability

Декларативное описание умения устройства. Объявление умения состоит из нескольких частей. 

**Meta**

Мета-информация возможности устройства. Каждое умение должно объявлять эту мета-информацию.   

Перечисляет набор поддерживаемых умением директив, генерируемые умением события. Также описывает флаг `retrievable`, 
отражающий возможность чтения состояния из поля `State` (о нем - далее), и возможность отсылать события (о них - тоже далее). 
```
    message TMeta {
        option (NYT.default_field_flags) = SERIALIZATION_YT;

        repeated TCapability.EDirectiveType SupportedDirectives = 1 [json_name = "supported_directives", (NYT.column_name) = "supported_directives"];
        repeated TCapability.EEventType SupportedEvents = 4 [json_name = "supported_events", (NYT.column_name) = "supported_events"];
        bool Retrievable = 2 [json_name = "retrievable", (NYT.column_name) = "retrievable"];
        bool Reportable = 3 [json_name = "reportable", (NYT.column_name) = "reportable"];
    }
```

**Parameters**

Иммутабельное (в рамках одной прошивки) описание ограничений на взаимодействие с умением

Примеры: 
1. [Границы управления яркостью лампочки](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L582)
2. [Список приложений, которые можно включать голосом на телевизоре с WebOS](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L766)
3. [Тип и границы бандов эквалайзера](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L845)
4. [Поддерживаемые форматы воспроизведения анимации на колонке](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L937) 

Параметры можно воспринимать как область допустимых значений для директив и состояний умения. 

**State**

Часть, описывающая текущее состояние умения. Все, что может изменяться и описывает текущее состояние,
должно храниться в этой части.

Примеры:  
1. [Включена/выключена лампочка](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L191)
2. [Какая цветовая температура выставлена на лампочке](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L702)
3. [Текущее состояние и пресет эквалайзера](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L886)
4. [Идентификатор текущего приложения на телевизоре](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L754)

**Directives**

Описание директив, с помощью которых можно взаимодействовать с устройством. 

Примеры:  
1. [Директива выключения/выключения света](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L195)
2. [Директива запуска приложения на телевизоре](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L770)
3. [Директива выставления бандов эквалайзера](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L904)
4. [Директива отрисовывания последовательности анимаций](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L950)

**Events**

Описание событий, генерируемых устройством.

Примеры:  
1. [Событие движения с датчика движение](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L1030)
2. [Событие обновления состояния умения](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/endpoint/capability.proto?rev=r9570516#L735) (может быть объявлено каждым умением)

#### Environment State
Датасорс, из которого можно получить информацию об эндпойнтах колонки и окружающих её устройств. 
Находится в файле [environment_state.proto](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/environment_state.proto?rev=r9137404#L64).
