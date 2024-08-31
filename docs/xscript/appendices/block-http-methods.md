# Методы HTTP-блока 

Входные параметры методов HTTP-блока могут конкатенироваться.

Если метод принимает несколько параметров, в первую очередь из списка параметров для конкатенации отбрасываются параметры Tag. Среди оставшихся параметров отбрасываются последние n-1 параметров, где n - количество параметров, которые принимает метод. Остальные параметры конкатенируются.

Если в заголовке `Content-Type` ответа формат возвращаемых данных описан как `text/json` или `application/json`, полученные данные интерпретируются как JSON и автоматически [преобразуются в XML](json-to-xml.md).

Во всех методах блока можно выставлять исходящие HTTP-заголовки с помощью тега [header](../reference/header.md).

**Список методов HTTP-блока**:
- [get](block-http-methods.md#get);
- [get_binary_page](block-http-methods.md#get_binary_page);
- [get_by_request](block-http-methods.md#get_by_request);
- [get_by_state](block-http-methods.md#get_by_state);
- [get_http](block-http-methods.md#get_http);
- [post](block-http-methods.md#post);
- [post_by_request](block-http-methods.md#post_by_request);
- [post_http](block-http-methods.md#post_http).

#### `get` {#get}

Синоним метода [get_http](block-http-methods.md#get_http).

#### `get_binary_page (getBinaryPage)` {#get_binary_page}

Получает по HTTP расположенный по указанному адресу бинарный файл и сообщает пользователю об окончании обработки страницы. В случае успешного выполнения блок формирует ответ вида:

```
<success content-type="application/pdf" url="http://www.ya.ru/1.pdf">1</success>
```

На странице этот ответ не отображается, но может быть использован при обработке страницы средствами XSL и XPath.

Если удаленный ресурс возвращает HTTP-заголовок `Content-type`, он добавляется в заголовки ответа XScript.

В случае успешного выполнения метода основное XSL-преобразование не накладывается.

**Входные параметры**: URL ресурса, к которому выполняется запрос.

**Пример использования**:

```
<http>
    <method>getBinaryPage</method>
    <param type="String">http://www.ya.ru/1.pdf</param>
</http>
```

#### `get_by_request (GetByRequest)` {#get_by_request}

Выполняет HTTP-запрос по указанному URL, при чем в качестве параметров запроса используются параметры из запроса страницы, который пришел в XScript.

**Входные параметры**: URL ресурса, к которому выполняется запрос.

**Пример использования**:

```
<http>
   <method>get_by_request</method>
   <param type="String">http://www.ya.ru/index.xml</param>
</http> 
```

#### `get_by_state (GetByState)` {#get_by_state}

Выполняет HTTP-запрос по указанному URL, при чем в качестве параметров запроса используются все переменные из [State](../concepts/state-ov.md). В качестве имени параметра выступает имя переменной, в качестве значения параметра - значение переменной.

**Входные параметры**: URL ресурса, к которому выполняется запрос.

**Пример использования**:

```
<http>
   <method>get_by_state</method>
   <param type="String">http://www.ya.ru/index.xml</param>
</http> 
```

#### `get_http (getHttp)` {#get_http}

Отправляет HTTP-запрос методом `GET` по указанному URL, принимает ответ сервера и размещает его на странице.

**Входные параметры**: составляющие URL ресурса, к которому выполняется запрос, задаваемые с помощью тегов [param](../reference/param.md) и [query-param](../reference/query-param.md).

Содержимое тегов [param](../reference/param.md) конкатенируется, полученное значение определяет базовую часть URL запроса. Содержимое тегов [query-param](../reference/query-param.md) добавляется к базовой части URL в качестве параметров.

**Пример использования**:

```
<http>
   <method>getHttp</method>
   <param type="String">http://devel.yandex.ru:</param>
   <param type="StateArg">port</param>
   <param type="String">/dir1/dir2/</param>
   <param type="String">example.xml</param> 
</http>
```

В приведенном примере все параметры последовательно конкатенируются, в результате чего методу будет передан один строковый параметр - URL "http://devel.yandex.ru:8090/dir1/dir2/example.xml".

#### `post` {#post}

Отправляет HTTP-запрос методом `POST` по указанному URL, принимает ответ сервера и размещает его на странице.

**Входные параметры**:

Метод принимает на вход неограниченное ненулевое число параметров, задаваемых с помощью тегов [param](../reference/param.md) и [query-param](../reference/query-param.md). При этом хотя бы один параметр должен быть задан с помощью тега [param](../reference/param.md).

Содержимое тегов [param](../reference/param.md) конкатенируется, полученное значение определяет URL запроса. Тело запроса формируется из содержимого тегов [query-param](../reference/query-param.md).

**Пример использования**:

```
<http method="post">
  <param type="StateArg" id="echo_host"/>
  <param type="String">/echorequest.xml</param>
  <query-param id="remote_ip" type="ProtocolArg"/>
  <x:meta xpointer="/x:meta/param[@name = 'URL']"/>
</http>
```

#### `post_by_request (postByRequest)` {#post_by_request}

Добавляет к указанному URL строку query основного запроса и выполняет по полученному адресу POST-запрос с телом основного запроса.

**Входные параметры**: URL ресурса, к которому выполняется запрос.

**Пример использования**:

```
<http>
    <method>postByRequest</method>
    <param type="String">http://devel.yandex.ru/example.xml</param>
</http>
```

#### `post_http (postHttp, postHTTP)` {#post_http}

Отправляет HTTP-запрос методом `POST` по указанному URL, принимает ответ сервера и размещает его на странице.

**Входные параметры**:

Метод принимает на вход неограниченное ненулевое число параметров, задаваемых с помощью тегов [param](../reference/param.md) и [query-param](../reference/query-param.md). При этом хотя бы один параметр должен быть задан с помощью тега [param](../reference/param.md).
- Если только один из параметров метода задан с помощью тега [param](../reference/param.md), то этот параметр интерпретируется как URL, к которому отправляется HTTP-запрос. Параметры, заданные с помощью тегов [query-param](../reference/query-param.md) конкатенируются и формируют тело POST-запроса.
- Если для задания параметров метода используется больше одного тега [param](../reference/param.md), то из содержимого всех тегов [param](../reference/param.md), кроме последнего, и содержимого всех тегов [query-param](../reference/query-param.md) формируется URL-запроса. Содержимое тегов [param](../reference/param.md) конкатенируется, затем к полученной строке добавляется содержимое тегов [query-param](../reference/query-param.md) в виде параметров запроса в URL-кодированном виде. Из последнего тега [param](../reference/param.md) формируется тело POST-запроса.

**Пример использования**:

```
<http timeout="10000" proxy="yes">
   <method>postHttp</method>
   <param type="String">http://mail.yandex.ru/api/colabook_addp</param>
   <param type="StateArg" as="String">contact_info</param>
</http>
```

### Узнайте больше {#learn-more}
* [HTTP-блок](../concepts/block-http-ov.md)
* [http](../reference/http.md)
* [query-param](../reference/query-param.md)