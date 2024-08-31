# Атрибуты XScript-блоков 

Атрибуты XScript-блоков влияют на процесс и результат их обработки.

Все атрибуты имеют два варианта записи: как атрибут тега и как свойство, то есть в виде отдельного тега. Например, атрибут `guard` можно указать следующими способами:

```
<mist guard="name_in_state">
    ...
</mist>
```

или

```
<mist>
    <guard>name_in_state</guard>
    ...
</mist>
```

Ниже перечислены атрибуты блоков XScript.

#|
|| Атрибут | Описание | Блоки, в которых может использоваться | Пример использования ||
|| _guard_ | Условное выполнение блока. Для того чтобы блок был выполнен, необходимо, чтобы переменная, имя которой присвоено атрибуту, присутствовала в контейнере [State](../concepts/state-ov.md) и имела значение, отличное от пустой строки и "0", или отсутствовало в State. Тег [\<guard\>](../reference/guard.md) обладает расширенными возможностями по сравнению с атрибутом `guard`: в нем могут использоваться переменные типов QueryArg, StateArg, Cookie, HttpHeader, UID, LiteUID, Login, LiteLogin, IsLite, VHostArg, ProtocolArg, RegName и LiteRegName. | Все | 
``` 
    <mist guard ="name_in_state">
     <method>set_state_string</method>
     <param type="String">1</param>
     <param type="QueryArg">req_param</param>
     </mist> 
 ``` 
||
|| _guard-not_ | 
guard-отрицание. 
Блок будет выполнен только в том случае, если переменная, имя которой присвоено атрибуту, присутствует в State и её значение ложно. Тег [\<guard-not\>](../reference/guard-not.md) обладает расширенными возможностями по сравнению с атрибутом `guard-not`: в нем могут использоваться переменные типов QueryArg, StateArg, Cookie, HttpHeader, UID, LiteUID, Login, LiteLogin, IsLite, VHostArg, ProtocolArg, RegName и LiteRegName. | Все | 

``` 
    <mist guard-not="name_in_state">
    <method>set_state_string</method>
     ...
 </mist> 
``` 
||
|| _id_ | Идентификатор блока. | Все | 

``` 
<block id="someblock">
    ...
 </block> 
``` 
||
|| _xmlns_ {#xmlns}| 
Пространство имен, которое будут использоваться в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута [xpointer](xpointer.md). | Все | 

``` 
<http xmlns:dd="http://www.yandex.ru/dd" xpointer="/tree/dd:subtree">
      <method>getHttp</method>
      <param type="StateArg">url</param>
 </http> 
 ```
 ||
|| _xpointer_ | XPointer-выражение, накладывающееся на результат работы блока. | Все | 
См. [Атрибут xpointer](xpointer.md) ||
|| _xslt_{#xslt} | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Все | См. [Перблочное XSL-преобразование](../concepts/per-block-transformation-ov.md) ||
|| _method_ | Имя метода, который должен быть вызван при обработке блока. | Auth-block, Banner-блок, CORBA-блок, Custom-morda-блок, File-блок, Geo-блок, HTTP-блок, Local-program-блок, Mist-блок, Mobile-блок, Tinyurl-блок | 
См. [method](../reference/method.md) ||
|| _elapsed-time_ {#elapsed-time} | Если данному атрибуту присвоено значение "yes", в корневой элемент результата работы блока будет добавлен атрибут elapsed-time со значением, указывающим время работы блока в секундах. | File-блок, HTTP-блок, CORBA-блок, Auth-блок, Local-блок, Tinyurl-блок, While-блок | 

``` 
<block elapsed-time="yes">
    ...
 </block> 
 ``` 
 ||
|| _threaded_{#threaded}| 
Если данному атрибуту присвоено значение "yes", блок обрабатывается асинхронно (в отдельном потоке). Значение "no" имеет смысл устанавливать в случае, когда необходимо выключить асинхронность обработки конкретного запроса при установленном атрибуте [all-threaded](../reference/xscript.md#all-treaded) ="yes". | File-блок, HTTP-блок, CORBA-блок, Auth-блок, Local-блок, Tinyurl-блок, While-блок | 

``` 
<block threaded="yes">
    ...
 </block> 
 ```
 ||
|| _timeout_ | Таймаут выполнения блока (в миллисекундах). По умолчанию равен 5000 миллисекундам (5 сек.). | File-блок, HTTP-блок, CORBA-блок, Auth-блок, Local-блок, Tinyurl-блок, While-блок | 

``` 
<block timeout="10000">
    ...
 </block> 
 ```
||
|| _no-cache_{#no-cache} | 
Категория пользователей, для которых запрещено кэширование данного блока. Может принимать значения "uid" (запрет кэширования для авторизованных пользователей) и "lite" (запрет кэширования для Lite-пользователей), а также оба эти значения, разделенные пробелом ("uid lite" - запрет кэширования и для авторизованных, и для Lite-пользователей). | CORBA-блок, HTTP-блок, File-блок, Local-блок, While-блок | 

``` 
<http no-cache="uid" tag="100"> 
``` 
||
|| _tag_{#tag} | 
Время кэширования результата работы блока (в секундах). В HTTP- и File-блоках может также иметь значение "yes", что означает неопределенное время кэширования (см. [Кэширование результатов работы XScript-блока](../concepts/block-results-caching.md)). | CORBA-блок, HTTP-блок, File-блок, Local-блок, While-блок | 

``` 
<block tag="60">
    <name>Yandex/Project/Example.id</name>
    <method>getSomeInfo</method>
    <param type="Request"/>
    <param type="StateArg">surname</param>
 </block> 
 ```
 ||
|| _remote-timeout_ | 
Время ожидания ответа на запрос от back-end-сервера (в миллисекундах). Если значение этого атрибута больше, чем атрибута `timeout`, то даже по истечении времени обработки блока, удаленный запрос продолжит обрабатываться, и его результат, если он будет получен, будет помещен в кэш. По умолчанию значение `remote-timeout` совпадает с `timeout`. | HTTP-блок, CORBA-блок | 

``` 
<block remote-timeout="15000">
    ...
 </block> 
 ```
 ||
|| _retry-count_ | 
Количество повторных попыток получения результата от удаленного сервера в случае ошибок соединения. Повторная попытка может быть сделана только до истечения `timeout`. Время выполнения попытки равно `remote-timeout`.

Значение по умолчанию: 0. | HTTP-блок, CORBA-блок | 


```
 <http timeout="15000" remote-timeout="5000" retry-count="2">
    ...
 </http> 
 ```
||
|| _encoding_ | Кодировка запрошенного по HTTP XML-документа. Из нее полученный документ переводится в кодировку UTF-8. | HTTP-блок | 

``` 
<http encoding="utf-8">
    ...
 </http> 
 ```
||
|| follow-redir | Если <q>yes</q>, то HTTP-блок следует редиректу. | HTTP-блок | 

``` 
<http follow-redir="yes">
    ...
 </http> 
 ```
 ||
|| _print-error-body_ {##print-error-body} | 
Если данному атрибуту присвоено значение "yes", то в случае HTTP-ответа со статусом 400-499 или 500-599, [сообщение об ошибке](../concepts/error-diag-ov.md) xscript_invoke_failed будет содержать тело ответа HTTP-сервера. | HTTP-блок | 

``` 
<http print-error-body="yes">
    ...
 </http> 
 ```
 ||
|| _proxy_ | 
**HTTP-блок**

Если атрибут имеет значение <q>yes</q>, XScript будет передавать в HTTP-запросе к третьей стороне HTTP-заголовки запроса, пришедшего от пользователя. Значение по умолчанию: "no".

**Local-блок**

Если атрибут имеет значение <q>yes</q>, [Local-блок](../concepts/block-local-ov.md) получает [доступ к родительским объектам](../concepts/block-local-ov.md#parent_context).

Если атрибут имеет значение <q>request</q>, [Local-блок](../concepts/block-local-ov.md) получает [доступ к родительскому объекту Request](../concepts/block-local-ov.md#request_access). | HTTP-блок, Local-блок | 

```
 <http proxy="yes">
    ...
 </http> 

 ```



 ``` <local proxy="request">
    ...
 </local> 
 ```
||
|| _ignore-not-existed_ | 
Если этому свойству присвоено значение "yes" и запрашиваемый файл отсутствует, то File-блок ничего не вернет, обработка страницы продолжится, а сообщение об отсутствии файла будет выведено в лог XScript с уровнем INFO. | File-блок | 

``` 

<file ignore-not-existed="yes">
    ...
 </file> 

 ```
 ||
|| _name_ | Имя CORBA-серванта, к которому выполняется запрос. | CORBA-блок | 
См. [name](../reference/name.md) ||
|| _nameref_ | Имя переменной в контейнере State, содержащей имя CORBA-серванта, к которому выполняется запрос. | CORBA-блок | 
См. [nameref](../reference/nameref.md) ||
|#

