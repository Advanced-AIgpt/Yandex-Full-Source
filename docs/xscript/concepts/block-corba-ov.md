# CORBA-блок

_CORBA-блок_ предназначен для вызова методов CORBA-сервантов.

CORBA-блок может выполняться асинхронно (если установлен атрибут [threaded="yes"](../appendices/attrs-ov.md#threaded)), а результаты его работы могут [кэшироваться](block-results-caching.md).

Для вызова метода CORBA-серванта необходимо указать:

- полное имя CORBA-компонента, к которому производится обращение ([name](../reference/name.md)), или имя переменной State, содержащей это имя ([nameref](../reference/nameref.md));
- имя метода выбранного компонента ([method](../reference/method.md));
- список передаваемых методу параметров ([param](../reference/param.md)). Набор передаваемых параметров должен соответствовать IDL компонента, к которому производится обращение, иначе произойдет [ошибка](error-diag-ov.md) времени выполнения.
    
    В качестве параметров методам CORBA-блока могут передаваться [объекты и структуры XScript](parameters-complex-ov.md).

При передаче в CORBA-блок параметра типа [StateArg](parameters-matching-ov.md#statearg), происходит неявное преобразование параметра к типу String, вне зависимости от того, какой тип имеет соответствующая переменная в контейнере [State](state-ov.md). Например:

```
<mist method="set_state_long">
     <param type="String">var</param>
     <param type="Long">15</param>
</mist>

<block method="some_method">
     <name>someServant.id</name>
     <param type="StateArg">var</param>  <!-- будет передана переменная типа String со значением "15" -->
     <param type="StateArg" as="Long">var</param>  <!-- будет передана переменная типа Long со значением 15 --> </block>
```

Существует возможность вызывать одинаковые методы на разных CORBA-сервантах.

Ожидается, что в ответ на CORBA-запрос будет возвращено значение [XMLString](https://svn.yandex.ru/websvn/wsvn/common/idl/trunk/xmlstring/idl/xmlstring.idl) - текстовое представление XML-документа в кодировке UTF-8.

В результате CORBA-вызова сервант может сгенерировать исключение. XScript обрабатывает только исключения `ServantFailed`, структура которых описана в [xscript.idl](https://svn.yandex.ru/wsvn/xscript/xscript-corba/trunk/idl/xscript.idl).

**Пример CORBA-блока**:

```
<block threaded="yes">
    <nameref>objname</nameref>
    <method>getSomeValue</method>
    <param type="String">1</param>
    <param type="QueryArg">req_param</param>
</block>
```

### Узнайте больше {#learn-more}
* [block](../reference/block.md)
