# XScript

Блок `lang-detect` реализован в модуле xscript-yandex.so. Библиотека устанавливается пакетом `xscript-yandex`.

Данные **Паспорта**, куки **my**, заголовка **Accept-Language** и **домен** блок забирает из текущего контекста исполнения. Это, в частности, означает, что блок **lang-detect** не может использоваться внутри **Local**-блока, если последний не имеет доступа к родительским объектам (т. е. **Local**-блок используется в режиме `proxy="no"`).

Описанная ниже функциональность также присутствует в пространстве имен [xscript.langdetect](https://doc.yandex-team.ru/XScript5/interface-developer-guide/appendices/block-js-xscript-langdetect-module.html) [JavaScript](https://doc.yandex-team.ru/XScript5/interface-developer-guide/concepts/block-js-ov.html)-блока.


## Подключение модуля {#xsctipt-linkup}

Для подключения модуля в конфигурационном файле XScript следует указать:

```xml
<modules>
...
    <module id="lang-detect">
        <path>/usr/lib/xscript/xscript-yandex.so</path>
    </module>
...
</modules>
```


## Методы {#methods}

**Список методов блока**:
- [find](#find);
- [list](#list);
- [findDomain](#findDomain);
- [cookie2language](#cookie2language);
- [language2cookie](#language2cookie).

**Входные параметры** методов `find` и `list`:

- Регион пользователя. Определяется с помощью метода `set_state_parents` **Geo**-блока;
    
- Перечень поддерживаемых сервисом языков;
- Язык сервиса по умолчанию (необязательно). Значение параметра не может быть пустым.

#### Метод `find` {#find}

Возвращает язык отображения страницы.

**Пример**:
```
<geo-block>
  <method>set_state_parents</method>
  <param type="String">parents</param>
  <!-- 236 - Набережные Челны -->
  <param type="StateArg" default="236">region</param> 
</geo-block>
<!-- Определяем наиболее подходящий язык -->
<x:lang-detect method="find">
  <param type="StateArg" id="parents"/>
  <param type="String">tt,ru,uk</param>
</x:lang-detect>
<!-- Выставляем язык сервиса по умолчанию, если не найден подходящий -->
<x:lang-detect method="find">
  <param type="StateArg" id="parents"/>
  <param type="String">en,be</param>
  <param type="String">be</param>
</x:lang-detect>
```

Результат (при обращении из московского офиса Яндекса):

```
<state type="Geo" name="parents">236,11119,40,225,10001,10000</state>
<!-- Определяем наиболее подходящий язык -->
<lang-detect-result-find>
  <lang id="ru" name="Ru"/>
</lang-detect-result-find>
<!-- Выставляем язык сервиса по умолчанию, если не найден подходящий -->
<lang-detect-result-find>
  <lang id="be" name="By"/>
</lang-detect-result-find>
```

#### Метод `list` {#list}

Возвращает список релевантных пользователю языков. Элементы списка отсортированы по атрибуту `name`.

**Пример**:

```
<geo-block>
  <method>set_state_parents</method>
  <param type="String">parents</param>
  <!-- 236 - Набережные Челны -->
  <param type="StateArg" default="236">region</param> 
</geo-block>
<!-- Получаем список релевантных пользователю языков -->
<x:lang-detect method="list">
  <param type="StateArg" id="parents"/>
  <param type="String">tt,ru,uk</param>
</x:lang-detect>
```

Результат (при обращении из московского офиса Яндекса):

```
<state type="Geo" name="parents">236,11119,40,225,10001,10000</state>
<!-- Получаем список релевантных пользователю языков -->
<lang-detect-result-list>
  <lang id="ru" name="Ru"/>
  <lang id="tt" name="Tat"/>
</lang-detect-result-list>
```

#### Метод `findDomain` {#findDomain}
Возвращает домен и регион пользователя.
**Входные параметры:**

- Регион пользователя;
- Перечень доменов для переадресации;
- Блок `cr` куки [куки yp](https://wiki.yandex-team.ru/cookies/y#yp).

**Пример:**
```
<geo-block>
    <method>set_state_parents</method>
    <param type="String">parents</param>
    <!-- 236 - Набережные Челны -->
    <param type="StateArg" default="236">region</param> 
</geo-block>
<!-- Получаем домен и регион пользователя -->
<x:lang-detect method="findDomain">
    <param type="StateArg" id="parents"/>
    <param type="String">ua,kz,by</param>
    <param type="String">ru</param>
</x:lang-detect>
```
Результат (при обращении к странице `http://mail.yandex.ru/neo2` из московского офиса Яндекса):
```
<state type="Geo" name="parents">
    236,11119,40,225,10001,10000
</state>
<!-- Получаем домен и регион пользователя -->
<lang-detect-result-find-domain changed="0" content-region="236">
    mail.yandex.ru
</lang-detect-result-find-domain>
```

#### Метод `cookie2language` {#cookie2language}

Преобразует числовой идентификатор языка в строковый.

**Входные параметры**:

- Целочисленный идентификатор языка из куки `my`.

**Пример:**
```
<x:lang-detect method="cookie2language">
    <param type="ULong">2</param>
</x:lang-detect>
```
Результат:
```
<lang-detect-result value="2">
   uk
</lang-detect-result>
```

#### Метод `language2cookie` {#language2cookie}

Преобразует строковый идентификатор языка в числовой.

**Входные параметры**:

- Строковый идентификатор языка.

**Пример:**
```
<x:lang-detect method="language2cookie">
    <param type="String">kk</param>
</x:lang-detect>

```
Результат:
```
<lang-detect-result id="kk">
    4
</lang-detect-result>
```

## Узнайте больше {#learnmore}

- [XScript5 Руководство разработчика интерфейсов](https://doc.yandex-team.ru/XScript5/interface-developer-guide/concepts/about.html)
