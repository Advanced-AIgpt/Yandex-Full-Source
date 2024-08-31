# Диагностика ошибок в XScript

## Результат работы блока в случае ошибки времени выполнения {#result}

Если при обработке любого [XScript-блока](block-ov.md) возникает ошибка времени выполнения, то в данном блоке вместо ожидаемого XML-фрагмента выводится сообщение об ошибке, обёрнутое XML-тегом `<xscript_invoke_failed>`. Ниже приведены примеры подобных ошибок:

```
<xscript_invoke_failed error="MARSHAL_PassEndOfMessage" block="block" id="543" method="FakeMethod" object="Yandex/Fake.id"/>

```

```
<xscript_invoke_failed error="bad arity" block="mist" method="set_state_long"/>

```

```
<xscript_invoke_failed error="server responded 404" block="http" method="getHttp" status="404" url="http://company.yandex.ru/a?b=38"/>

```

```
<xscript_invoke_failed reason="UNKNOWN"
   object="Yandex/Example/Example :
      exampleRequestAuth /usr/local/www/assessor-ng/test.xml"/>
```

`<xscript_invoke_failed>` содержит тип блока, в котором произошла ошибка, id и метод блока, если таковые присутствуют, а также дополнительную информацию, специфичную для блока данного типа. Например, для HTTP-блока будет выведен запрашиваемый URL и статус ответа, для CORBA-блока - имя серванта и т.д.

В режиме "development" информация о вызове CORBA-блоков более детализирована. Выводятся поля `minor` при перехвате `CORBA::SystemException`, а также `host`, `reason` и `additional` при перехвате исключения от серванта`Yandex::XScript::ServantFailed`.

Вся информация из тега `<xscript_invoke_failed>` дублируется в логах XScript.

В случае ошибки XPath-выражение не выполняется, перблочное XSL-преобразование не накладывается, а также не производится обработка секции [\<meta\>](meta.md).

## Обработка ошибок XSL {#xsl-error}

В режиме _development_ любая ошибка XSL считается критичной и приводит к генерации исключения `<xscript_invoke_failed>` в перблочном XSL-преобразовании, или к ошибке с кодом 500 в основном XSL-преобразовании. В режиме _production_ сообщение об ошибке записывается в лог, `<xscript_invoke_failed>` или ошибка с кодом 500 не генерируются.

Элемент _\<xsl:message\>_ позволяет передать из основного или перблочного XSL-преобразования сообщение об ошибке для записи в лог с уровнем `err`. При этом страница или блок не кэшируются.

Кроме того, реализовано логирование сообщений через XSL при помощи XSL-функций [log-info](../appendices/xslt-functions.md#log-info), [log-warn](../appendices/xslt-functions.md#log-warn) и [log-error](../appendices/xslt-functions.md#log-error).

## Ошибки при выполнении секции \<meta\> {#meta-error}

В случае возникновения ошибок при обработке секции [\<meta\>](meta.md) возвращается ошибка `<meta_invoke_failed>`. При этом блок выполняется как обычно.

## Логи {#logs}

Если в [конфигурационном файле](../appendices/config.md) XScript-а не указана настройка [file](../appendices/config-params.md#file), логи работы XScript записываются в `syslog`.

