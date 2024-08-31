# Lang-detect-блок

_Lang-detect-блок_ предназначен для автоматического определения языка отображения страницы.

Определение языка производится на основании данных HTTP-запроса. При этом учитываются следующие параметры:

- настройка пользователя в [куке my](http://wiki.yandex-team.ru/MyCookie) (второй элемент [39 блока](http://wiki.yandex-team.ru/MyCookie/NomerBloka));
- значение HTTP-заголовка Accept-Language;
- домен первого уровня (TLD), на котором находится страница;
- числовые идентификаторы региона и его родителей в геобазе (определяются с помощью метода [set_state_parents](../appendices/block-geo-methods.md#set_state_parents) Geo-блока);
- настройка пользователя в passport (не учитывается в настоящий момент);
- Блок `cr` куки [`куки yp`](https://wiki.yandex-team.ru/cookies/y#yp).

Методика определения языка отображения страницы описана на странице [http://wiki.yandex-team.ru/portal/international/lang](http://wiki.yandex-team.ru/portal/international/lang#logikavyborajazykaotobrazhenijastranicy).

Блок позволяет определить язык отображения страницы и получить список [релевантных пользователю языков](http://wiki.yandex-team.ru/portal/international/lang#logikapoluchenijaspiskarelevantnyxpolzovateljujazykov).

**Пример использования блока Lang-detect**:

```
<x:lang-detect method="find">
  <param type="StateArg" id="parents"/>
  <param type="String">tt,ru,uk</param>
</x:lang-detect>
```

Результат:

```
<lang-detect-result-find>
  <lang id="ru" name="Ru"/>
</lang-detect-result-find>
```

Данные из куки my, заголовок Accept-Language и TLD блок автоматически забирает из текущего контекста исполнения. Это, в частности, означает, что блок Lang-detect не может использоваться внутри Local-блока, если последний не имеет [доступа к родительским объектам](block-local-ov.md#parent_context) (т. е. Local-блок используется в режиме proxy="no").

**Пример. Недоступность HTTP-заголовков**:

```
<x:local><root> <!-- по умолчанию используется режим proxy="no" -->
 <x:local proxy="yes"><root> <!-- HTTP-заголовки потерялись -->
  <x:lang-detect method="find"> <!-- fail -->
    <param type="StateArg" id="parents"/>
    <param type="String">ru,uk,be</param>
  </x:lang-detect>
 </root></x:local>
</root></x:local>
```

Результат:

```
<xscript_invoke_failed error="lang detect error : root context not allowed (must x:local proxy=yes)" block="lang-detect" method="find"/>
```

### Узнайте больше {#learn-more}
* [Методы Lang-detect-блока](../appendices/block-lang-detect-methods.md)
* [lang-detect](..//reference/lang-detect.md)