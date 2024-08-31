# JavaScript-блок

XScript 5 предоставляет возможность включать в XScript-страницы исполняемый код на языке JavaScript. Такой код должен размещаться внутри XML-элемента `js`, при этом использование пространства имен `http://www.yandex.ru/xscript` обязательно.

```xml
<?xml version="1.0" encoding="utf-8"?>
<page xmlns:x="http://www.yandex.ru/xscript">
  <x:js>
    var hello = 'Hello JS';
    xscript.print(hello);
  </x:js>
</page>
```

На этапе компиляции XScript-страницы код JavaScript-блоков конвертируется во внутренний формат и исполняется. Если во время исполнения скрипта возникли ошибки, процесс обработки страницы прерывается и возвращается код ошибки 500.

На каждый запрос XScript создает JS-контекст, в котором исполняется код всех JavaScript-блоков.

#|
|| Код страницы | Результат выполнения ||
|| 
```xml 
<page xmlns:x="http://www.yandex.ru/xscript">
 <x:js>
   hello = 'привет, JS';
   var last = '!';
   xscript.print(hello + last);
 </x:js>
 
 <x:js>
   sayHi(hello);
   function sayHi(str) {
     var last = '?';
     var a = str.split(', ').reverse().join(' ') + last;
     xscript.print(a.replace('привет', 'hello'));
   } 
 </x:js>
 
 <x:js>
   sayHi('снова привет' + last);
 </x:js>
 </page> 
 ```
| 

```xml 
<page xmlns:x="http://www.yandex.ru/xscript">
 <js>привет, JS!</js>
 <js>JS hello?</js>
 <js>снова hello!?</js>
 </page> 
 ```
||
|#

В дочерних XScript-контекстах с полным проксированием (`<x:local proxy="yes">...</x:local>`, `<x:while>...<x:while>`, `<file><method>invoke</method>...</file>`) используется родительский JS-контекст.

Если JavaScript-блок находится внутри Local-блока, не имеющего [доступа к родительским объектам](block-local-ov.md#parent_context) (`proxy="no" или proxy="request`"), для него создается новый JS-контекст.

Включать код JavaScript в XScript-документ нужно таким образом, чтобы не нарушалась грамматика XML. Это означает, что при необходимости использовать в JavaScript-коде специальные символы, следует либо заменять их на сущности, либо размещать весь код JavaScript-блока в разделе `<![CDATA[ ... ]]>`.

#|
|| Код страницы | Результат выполнения ||
|| 
```xml 
<page xmlns:x="http://www.yandex.ru/xscript">
 <x:js xpointer="text()">
 var name = "M&amp;M's (named after "
   + "the surnames of the company founders "
   + "Mars &amp; Murrie brothers)";
 </x:js>
 <x:js xpointer="text()">
 <![CDATA[
   var description = 'are dragée-like '
   + '<em>colorful button-shaped candies</em> '
   + 'produced by Mars, Incorporated.';
 ]]>
 </x:js>
 <x:js name="div">xscript.print(name + ' ' + description);</x:js>
 </page> 
 ``` 
 | 

 ```xml 
 <page>
   <div>M&M's (named after the surnames of the company founders Mars & Murrie brothers) are dragée-like <em>colorful button-shaped candies</em> produced by Mars, Incorporated.
   </div>
 </page> 
 ```
 ||
 |#

## Функции и объекты XScript {#function-and-obj}

XScript расширяет JavaScript набором функций и структур, предоставляющих доступ к программным компонентам и внутренним [базовым объектам](auth-ov.md) XScript. Эти функции и структуры определены в пространстве имен `xscript`. Обращение к ним производится так же, как если бы они являлись полями глобального JavaScript-объекта `xscript` и/или полями его внутренних объектов.

Структура `xscript` и ее дочерние структуры контейнерного типа, определяемые XScript, не являются полноценными JavaScript-объектами. Будем называть их **псевдообъектами** . К функциям следует относиться не как к полям объектов, а как к функциям, находящимся в соответствующих пространствах имен.

Псевдообъекты не допускают переопределение собственных полей, если они являются функциями и определены движком XScript (т.е. невозможно, например, переопределить поле (функцию) `xscript.print`). **Расширять псевдообъекты собственными полями настоятельно не рекомендуется**.

Псевдообъект `xscript` создается для каждого JS-контекста.

## Доступ к объектам XScript {#access-to-obj}

Доступ к базовым объектам XScript предоставляют следующие псевдообъекты JavaScript:
- [xscript.params](../appendices/block-js-xscript-params-object.md) — доступ к параметрам JavaScript-блока;
- [xscript.localargs](../appendices/block-js-xscript-localargs-object.md) — доступ к параметрам родительского Local-блока;
- [xscript.state](../appendices/block-js-xscript-state-object.md) — доступ к [контейнеру State](state-ov.md);
- [xscript.request.args](../appendices/block-js-xscript-request-args-object.md), [xscript.request.argArrays](../appendices/block-js-xscript-request-argarrays-object.md) — доступ к query-параметрам HTTP-запроса;
- [xscript.request.cookies](../appendices/block-js-xscript-request-cookies-object.md) — доступ к кукам HTTP-запроса;
- [xscript.request.headers](../appendices/block-js-xscript-request-headers-object.md) — доступ к заголовкам HTTP-запроса.

Для этих псевдообъектов определены операции итерирования по полям и чтения значений полей (`for <key> in xscript.<pseudo_object>`). Для полей псевдообъекта [xscript.state](../appendices/block-js-xscript-state-object.md) доступны также операции изменения и удаления свойств, что равносильно операциям изменения и удаления соответствующих элементов [контейнера State](state-ov.md).

Операция над полем псевдообъекта равносильна такой же операции над соответствующем полем связанного объекта XScript.

Любой псевдообъект контейнерного типа может быть преобразован в формат JSON с помощью функции [JSON.stringify](https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/JSON/stringify).

**Для псевдообъектов контейнерного типа недоступна операция проверки наличия ключа с помощью синтаксиса** `<key> in xscript.<pseudo_object>`. Значением этого выражения всегда будет `false`. Такие выражения легко заменяются на if-проверки типа `if (xscript.<pseudo_object>.<key>)` или строгим сравнением с undefined. Например, 
```javascript
if (xscript.request.args.cgiparam){...}
```
или
```javascript
if (xscript.localargs.key != undefined){...}
```

## JavaScript-функции XScript {#js-function}

Пространство имен [xscript](../appendices/block-js-xscript-methods.md) — функции общего назначения.

Пространство имен [xscript.logger](../appendices/block-js-xscript-logger-methods.md) — запись в лог сообщений различного уровня критичности.

Пространство имен [xscript.request](../appendices/block-js-xscript-request-methods.md) — извлечение информации из HTTP-запроса.

Пространство имен [xscript.response](../appendices/block-js-xscript-response-methods.md) — управление HTTP-ответом.

Класс [xscript.cookie](../appendices/block-js-xscript-cookie-methods.md) — работа с заданной кукой.

## Вывод результатов в результирующий XML-документ {#output-result}

Для вывода результатов работы JavaScript-блока предназначены функции [xscript.print](../appendices/block-js-xscript-methods.md#print) и [xscript.xmlprint](../appendices/block-js-xscript-methods.md#xmlprint).

Каждой из этих функций соответствует собственный поток вывода. Во время вызова функции, ее результат записывается в поток. В конце исполнения блока эти потоки направляются в результирующий документ. Сначала выводится поток функции `xscript.print`, затем поток `xscript.xmlrint`.

Поток `xscript.print` формирует текстовый узел, все специальные символы экранируются.

Поток `xscript.xmlprint` сериализуется в XML и валидируется.

#|
|| Код страницы | Результат выполнения ||
|| 
```xml 
<page xmlns:x="http://www.yandex.ru/xscript">
 <x:js>
 <![CDATA[
   xscript.xmlprint('<xmlprint>1</xmlprint>');
   xscript.print('<print>1');
   xscript.xmlprint('<xmlprint>2</xmlprint>');
   xscript.print('print 2');
 ]]>
 </x:js>
 </page> 
 ``` 
 | 
 ```xml 
 <page xmlns:x="http://www.yandex.ru/xscript">
 <js>&lt;print&gt;1
 print 2<xmlprint>1</xmlprint><xmlprint>2</xmlprint></js>
 </page> 
 ```
||
|#

## Обработка ошибок и предупреждений {#errors-handing}

Во время исполнения блока могут гененерироваться два типа сообщений: ошибки и предупреждения.

Ошибки могут быть перехвачены конструкцией `try/catch` и обработаны стандартным образом средствами JavaScript. Неперехваченная ошибка приводит к неудачному завершению блока с выводом в лог сообщения уровня error c текстом <q>JS ERROR <информация об ошибке></q>.

Предупреждения не перехватываются конструкцией `try/catch`, но приводят к записи в лог сообщения уровня warn с текстом <q>JS WARN <текст предупреждения></q>. В режиме development появление предупреждения также приводит к тому, что блок завершается неудачей.

При работе в режиме development все неперехваченные ошибки и предупреждения приводят к прерыванию процесса обработки страницы с кодом ошибки 500.

## Рекомендации по повышению производительности {#recs}

Для уменьшения времени выполнения JavaScript-блока, придерживайтесь следующих рекомендаций:

1. Не используйте на одной XScript-странице одновременно JavaScript- и Lua-блоки.
1. Не отключайте без необходимости проксирование данных (`<x:local proxy="request">` или `<x:local proxy="no">`).

Помните, что **создание JS-контекста затратно**.

