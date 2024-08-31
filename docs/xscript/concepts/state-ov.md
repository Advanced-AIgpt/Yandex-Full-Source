# Объект State: контейнер данных на время обработки запроса

## Определение {#definition}

_Yandex::State_ является одним из базовых объектов XScript и представляет собой контейнер данных.

State обладает семантикой карты или словаря и состоит из ячеек данных (пар "имя-значение"). В него можно добавлять данные, а затем модифицировать и извлекать с помощью соответствующих методов.

Если средствами [XScript-блоков](block-ov.md) в контейнер можно складывать только простые значения (строковые, численные), то средствами CORBA-компонентов, принимающих контейнер в качестве параметра, в него можно помещать практически любые виды данных: структуры, ссылки и проч.

Интерфейс объекта описан в [state.idl](https://svn.yandex.ru/wsvn/xscript/xscript-corba/trunk/idl/state.idl).


## Особенности работы {#wrokdetails}

State служит для передачи данных между блоками и CORBA-компонентами через XScript.


## Особенности использования {#usagedetails}

Для работы с контейнером используются главным образом [Mist-блок](block-mist-ov.md), имеющий [ряд методов](../appendices/block-mist-methods.md) для добавления и модификации значений в State.

Кроме того, доступ к данному объекту в XScript осуществляется либо через параметр [объектного типа](parameters-complex-ov.md) State, либо через специальный параметр-адаптер [StateArg](parameters-matching-ov.md#statearg).


## Время жизни {#lifetime}

Время жизни State обычно соответствует времени работы с запросом. В случае использования [Tag](tag-ov.md) время жизни State увеличивается.


## Пример использования {#example}

```
<block>
   <name>Yandex/Blogs/BloggerAux.id</name>
   <method>GetSessionToState</method>
   <param type="State"/>
   <param type="StateArg">fromsession</param>
   <param type="String">request</param>
   <param type="String"/>
</block>
```

* [Как передать данные между блоками или CORBA-компонентами](../tasks/how-to-transfer-data-between-servers.md)
* [Все типы параметров методов, вызываемых в XScript-блоках](../appendices/block-param-types.md)