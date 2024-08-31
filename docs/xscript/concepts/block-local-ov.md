# Local-блок

_Local-блок_ позволяет создать дочерний скрипт в теле XML-файла, не вынося его в отдельный файл. Скрипт может быть выполнен в своем собственном контексте, либо в родительском контексте, т. е. в контексте обработки файла, в котором он описан.

Блок может выполняться асинхронно (если установлен атрибут [threaded="yes"](../appendices/attrs-ov.md#threaded)).

**Пример Local-блока**:

```
<page xmlns:x="http://www.yandex.ru/xscript">
    <mist method="set_state_long">
        <param type="String">long_var</param>
        <param type="Long">100</param>
    </mist>
    <x:local>
        <!-- в LocalArgs будет записана переменная name типа String со значением "my_name" -->
        <param id="name" type="String">my_name</param>
        <!-- здесь в LocalArgs будет записана переменная var типа Long со значением 100 -->
        <param id="var" type="StateArg">long_var</param> 
        <root>
            <mist method="set_state_string">
                <param type="String">local_name</param>
                <param type="LocalArg">name</param>
            </mist>
            <mist method="dumpState"/>
        </root>
    </x:local>
</page>
```

В теге `<local>` обязательно должно быть указано пространство имен `http://www.yandex.ru/xscript`. Если пространство имен не указано, тег `<local>` будет восприниматься как обычный XML-тег и Local-блок не будет создан.

Блок содержит параметры (`<param>`), в которых описываются передаваемые скрипту параметры, с обязательным атрибутом `id` (имя параметра) и тег [\<root\>](../reference/root.md), внутри которого описывается тело скрипта.

Для того чтобы поменять имя корневого XML-элемента в ответе блока, необходимо указать в теге `<root>` атрибут `name="нужное_имя_элемента"`. При этом в входном XML атрибут `name` присутствовать не будет.

```
<root name="page">
```

Кроме того, существует возможность задать пространство имен корневого XML-элемента в ответе блока. Для этого пространство имен должно быть определено в теге Local-блока и указано в имени корневого элемента.

```
<x:local xmlns:g="http://www.ya.ru">
     <root name="g:page">
         ......
     </root>
</x:local>
```


## Контейнер LocalArgs {#localargs}

Параметры блока (`<param>`) передаются в скрипт, где из них создается доступный только для чтения типизированный контейнер _LocalArgs_. Значение атрибута `id` используется как имя переменной, а тело параметра - как значение. Если в качестве типа параметра указан [StateArg](parameters-matching-ov.md#statearg), то при отсутствии спецификатора `as` информация о типе переменной из контейнера [State](state-ov.md) будет сохранена и в LocalArgs.

Доступ из скрипта к контейнеру LocalArgs осуществляется через параметр типа [LocalArg](parameters-matching-ov.md#localarg), который может использоваться в [\<guard\>](../reference/guard.md), [\<guard-not\>](../reference/guard-not.md), [\<xslt\>](../reference/xslt-tag.md) и [\<xpointer\>](../reference/xpointer-tag.md).

Существует несколько способов работы с контейнером LocalArgs:

1. С помощью методов [get](../appendices/block-lua-localargs-methods.md#get), [has](../appendices/block-lua-localargs-methods.md#has) и [is](../appendices/block-lua-localargs-methods.md#is) Lua-объекта localargs.
1. С помощью XSL-функции [get-local-arg](../appendices/xslt-functions.md#get-local-arg).
1. С помощью методов Mist-блока [set_state_by_local_args](../appendices/block-mist-methods.md#set_state_by_local_args) и [echo_local_args](../appendices/block-mist-methods.md#echo_local_args).


## Контекст выполнения блока {#context}

По умолчанию блок выполняется в собственном (изолированном) контексте (`proxy="no"`). Это означает, что скрипт имеет пустой Request и свой собственный контейнер State. При этом CORBA-объекты _Request_ и _CustomMorda_ представляют собой нулевые ссылки, структра _RequestData_ пустая. То же самое относится к объектам авторизации: CORBA-объекты _Auth_, _LiteAuth_, _SecureAuth_ - нулевые ссылки, UID равен нулю, логин - пустой. CORBA-объект State - полноценный, вновь созданный.

Алгоритмы некоторых блоков (например, [Geo-блок](block-geo-ov.md), [блок Сustom-morda](block-custom-morda-ov.md), [Mobile-блок](block-mobile-ov.md)) требуют наличия доступа к родительским объектам. Для предоставления блоку доступа к таким объектам используется атрибут `proxy`.

### Доступ к родительским объектам {#parent_context}

Для того чтобы блок получил доступ к родительским объектам, необходимо в теге `<x:local>` указать атрибут `proxy="yes"`. В этом случае объекты _Request_, _State_, _CustomMorda_, _Auth_ и т.д. передаются из родительского контекста по ссылке.

```
<x:local proxy="yes">
    ...
</x:local>
```

### Доступ к родительскому объекту Request {#request_access}

Выполнение Local-блока с полным доступом к родительским объектам может вызвать затруднения, если [блок обрабатывается асинхронно](request-handling-file.md#async). Если блок имеет доступ к изменению родительских объектов, передаваемых по ссылке, возникает необходимость синхронизации изменений, производимых над этими объектами.

Для решения большинства задач достаточно, чтобы Local-блок имел доступ к объекту _Request_. Чтобы предоставить блоку возможность оперировать данными родительского объекта _Request_ необходимо присвоить атрибуту `proxy` значение <q>request</q>.

```
<page xmlns:x="http://www.yandex.ru/xscript">

<!-- 1 a=1 -->
<lua>a=1; print(xscript.state:setLong("a1", a))</lua>

<!-- 2 proxy: request a+=20 -->
<x:local proxy="request">
<root>
    <lua>
        <![CDATA[
            print('was:' .. a); a = a + 20; print(a)
            return '<x>' .. a .. '</x>'
        ]]>
    </lua>
</root>
<xpath expr="/root/lua/x" result="a2"/>
</x:local>

<!-- 3 a=1 -->
<lua>print(xscript.state:setLong("a3", a))</lua>

<!-- a1=1 a2=21 a3=1 -->
<mist method="dumpState"/>

</page>
```

Результат:

```
<?xml version="1.0" encoding="utf-8"?>
<page xmlns:x="http://www.yandex.ru/xscript">

<!-- 1 a=1 -->
<lua>1</lua>

<!-- 2 proxy: request a+=20 -->
<root xml:base="/usr/local/www/devel/lr.xml">
    <lua>was:1
        21<x>21</x>
    </lua>
</root>

<!-- 3 a=1 -->
<lua>1</lua>

<!-- a1=1 a2=21 a3=1 -->
<state_dump>
    <param type="Long" name="a1">1</param>
    <param type="String" name="a2">21</param>
    <param type="Long" name="a3">1</param>
</state_dump>

</page> 
```

При этом также как и в случае `proxy="no"` работает автоочистка временных переменных в State и Lua.
 Объект Response экранирован от глобального, т. е. нет необходимости в использовании redirect.


## Кэширование результатов работы блока {#cache}

Результаты работы блока могут [кэшироваться](block-results-caching.md).

Если блок был вызван с атрибутом [tag](../appendices/attrs-ov.md#tag)="yes", можно установить время валидности кэша с помощью метода Lua-блока [setExpireDelta](../appendices/block-lua-other-methods.md#set-expire-delta), которому передается время кэширования в секундах.

```
<x:local proxy="yes" tag="yes">
     <param id="name" type="String">my_name</param>
     <param id="var" type="StateArg">long_var</param>
     <root name="page">
         <mist method="set_state_string">
             <param type="String">local_name</param>
             <param type="LocalArg">name</param>
         </mist>
         <mist method="dumpState"/>
         <!-- Результаты работы Local-блока будут закешированы на 60 сек -->
         <lua>xscript.setExpireDelta(60)</lua> 
     </root>
</x:local>
```


## XSL-преобразования в Local-блоке {#xsl}

Основное XSL-преобразование на результаты работы скрипта не накладывается, поэтому указывать `<?xml-stylesheet ...>` в теле скрипта не имеет смысла.

При необходимости наложить XSL, следует использовать [перблочное преобразование](per-block-transformation-ov.md). Перблочное преобразование накладывается в контексте выполнения блока, т.е. в нем доступны те же те же _LocalArgs_, _Request_, _State_, _CustomMorda_, _Auth_, что и в выполняемом скрипте.

### Узнайте больше {#learn-more}
* [local](../reference/local.md)