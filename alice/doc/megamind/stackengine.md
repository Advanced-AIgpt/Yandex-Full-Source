# Паровоз

Паровоз это технология которая позволяет для пользователя объединять в цепочку воспроизведения разные сценарии Алисы.

Примером использования технологии является сценарий утреннее шоу: исходный сценарий подготавливает "порцию" контента 
(например новости и/или музыка), который проигрывается на клиенте, после чего необходимо вернуться в сценарий за новым 
контентом.

На уровне протокола сценария эта возможность реализуется через обмен сообщениями между сценариями через стек 
(StackEngine).

## Изменения в сценарном протоколе

Для управления стеком со стороны сценария есть две директивы:

### ResetAdd
[Описание в протоколе](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/stack_engine.proto?rev=r8044201#L24) 

Данная директива сначала сбрасывает фреймы своего сценария выкидывая их по одному сверху вниз, а дойдя до другого 
сценария остановится и сверху добавит в стек новые элементы.

Также у данной директивы есть поле RecoveryAction, где можно указать callback, с помощью которого можно восстановить 
работу при ошибках стека.

### NewSession
[Описание в протоколе](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/stack_engine.proto?rev=r7677239#L32) 
  
Данная директива создает новую сессию стека, полностью очищая старый. RequestId запроса, который инициировал новую 
сессию будет записываться в analytics_info в [ParentRequestId](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/analytics/analytics_info.proto?rev=r7677239#L26) 
на каждый **get_next**.

**Важно**: по ParentRequestId считаются границы TLT. Если не проставить NewSession, TLT вашего сценария может быть
атрибутирован другому.

**Важно**: данная директива должна идти первой в списке [Actions](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/stack_engine.proto?rev=r7677239#L37) 
стека. 

[Пример](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/alice_show/alice_show.cpp?rev=r7724098#L333), где шоу использует это поле и не начинает новую сессию.

### Поля в запросе

Чтобы ориентироваться, нужно ли начинать новую сессию или продолжать текущую в сценарий в запросе приходят следующие
поля:

#### [RequestSource](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/request.proto?rev=r7710902#L481)

Это поле принимает два значения:
* Default - обычный запрос
* GetNext - запрос в сценарий, получен из стека посредством запроса с серверной директивой `get_next`

#### [IsStackOwner](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/request.proto?rev=r8185286#L538)

Поле IsStackOwner указывает сценарию, что сейчас сессия стека принадлежит этому сценарию. Т.е. последний NewSession 
вызвал этот сценарий. 

## GetNext

GetNext - серверная директива Мегамайнда. При запросе с помощью данной директивы берется элемент с верхушки стека и 
дальше Мегамайнд обрабатывает запрос с этим элементом по обычному пайплайну. Данная директива автоматически добавляется 
к ответу Мегамайнда, если состояние стека изменилось.

```json
{
    "event": {
        "name": "@@mm_stack_engine_get_next",
        "payload": {
            "@recovery_callback": {
                "name": "recovery",
                "payload": {
                    "@scenario_name": "_scenario_",
                    "data": "value"
                },
                "ignore_answer": false,
                "type": "server_action",
                "is_led_silent": false
            },
            "@scenario_name": "_scenario_"
        },
        "type": "server_action"
    }
}
```
**Важно**: поля, начинающиеся с одного символа `@` - приватные и заполняются на стороне Мегамайнда.

### Ошибки, связанные с GetNext

* С клиента может прийти запрос с GetNext, но принадлежащий другой сессии.
* Также возможна ситуация, когда по разным причинам стек пуст, но с клиента пришел запрос с GetNext.

В обоих случаях Мегамайнд попробует восстановить стек, используя RecoveryCallback. Если же это не удастся, запрос 
завершится ошибкой.

## Кейсы
Ниже представлены примеры разных кейсов с использованием паровоза.

![stackengine-simple](../images/stackengine-simple.png)
