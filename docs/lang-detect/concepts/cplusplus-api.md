# С++

В библиотеке определено пространство имён `langdetect`, содержащее все её классы и структуры данных.

Основной функционал библиотеки реализован в классе `lookup`.

При компиляции проекта следует линковаться с библиотекой (`.so`) `yandex-lang-detect`. Заголовочные файлы библиотеки при установке помещаются в `/usr/include/langdetect`.

Библиотека устанавливается пакетами `libyandex-lang-detect`, `libyandex-lang-detect-dev` и `lang-detect-data.txt`.

Структуры:

Название | Описание
----- | -----
[find_domain_result](#struct-find_domain_result) | Предназначена для хранения результата работы метода `lookup:find_domain_ex`.
[langinfo](#struct-langinfo) | Предназначена для хранения информации о языке.


Классы:

Название | Описание
----- | -----
[userinfo](#class-userinfo) | Содержит информацию о пользователе.
[filter](#class-filter) | Содержит список языков сервиса.
[domaininfo](#class-domaininfo) | Содержит информацию о домене и регионе пользователя.
[domain_filter](#class-domain_filter) | Хранит список доменов для переадресации.
[lookup](#class-lookup) | В классе реализован функционал библиотеки по выбору домена и региона пользователя, языка отображаемой страницы и списка релевантных пользователю языков.


Порядок применения продемонстрирован [примером](#example).


## Структура find_domain_result {#struct-find_domain_result}

Содержит результат работы метода [lookup::find_domain_ex](#find_domain_ex).

Поле | Описание
----- | -----
`domain` | Наименование домена.
`content_region` | Идентификатор региона.
`found` | True, если логикой библиотеки найден домен для переадресации.



## Структура langinfo {#struct-langinfo}

Содержит информацию о языке.

Поле | Описание
----- | -----
`code` | Обозначение языка (например, uk — украинский).
`name` | Название языка (например, Ua).
`cookie_value` | Числовой идентификатор языка в формате куки my.



## Класс userinfo {#class-userinfo}

Содержит информацию о пользователе.

**Методы класса:**

Метод | Описание
----- | -----
[parse_accept_language](#parse_accept_language) | Разбирает поле Accept-Language заголовка HTTP-пакета и сохраняет результат во внутренней структуре. Получить результат можно вызовом `accept_language`.
[accept_language](#accept_language) | Выдает результат работы метода `parse_accept_language`.
[parse_geo_regions](#parse_geo_regions) | Разбирает регион пользователя и сохраняет результат во внутренней структуре. Получить результат можно вызовом `geo-regions`.
[geo-regions](#geo-regions) | Возвращает регион пользователя.
[parse_cookie](#parse_cookie) | Разбирает настройки языка интерфейса из куки my и сохраняет результат во внутренней структуре. Получить результат можно вызовом `cookie_value`.
[cookie_value](#cookie_value) | Выдает идентификатор языка в формате куки my.
[parse_host](#parse_host) | Разбирает URL, к которому обратился пользователь, извлекает из него домен и сохраняет результат во внутренней структуре. Получить результат можно вызовом `top-level-domain`.
[top-level-domain](#top-level-domain) | Возвращает домен верхнего уровня, к которому обратился пользователь.
[set-geo-regions](#set-geo-regions) | Сохраняет в классе данные о регионе пользователя.
[set_pass_language](#set_pass_language) | Сохраняет в классе [значение](source-data.md) языка пользователя из Паспорта.


### Метод parse_accept_language {#parse_accept_language}

Разбирает поле Accept-Language заголовка HTTP-пакета и сохраняет результат во внутренней структуре. Получить результат можно вызовом [accept_language](#accept_language).

```cpp
bool parse_accept_language(std::string const &str)
```

**Параметры**:

Параметр | Описание
----- | -----
str | Поле Accept-Language заголовка HTTP-пакета.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод accept_language {#accept_language}

{% note info %}

Устаревший метод. Оставлен для совместимости.

{% endnote %}


Выдает результат работы метода [parse_accept_language](#parse_accept_language).

```cpp
std::string const& accept_language() const
```

**Возвращаемое значение:**

Строка с перечнем языков.

### Метод parse_geo_regions {#parse_geo_regions}

Разбирает регион пользователя и сохраняет результат во внутренней структуре. Получить результат можно вызовом [geo-regions](#geo-regions).

```cpp
bool parse_geo_regions(std::string const &str)
```

**Параметры**:

Параметр | Описание
----- | -----
`str` | Регион пользователя в виде списка числовых идентификаторов региона и его родителей.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод geo-regions {#geo-regions}

Возвращает регион пользователя.

```cpp
std::list<int32_t> const& geo_regions() const
```

**Возвращаемое значение:**

Регион пользователя в виде списка числовых идентификаторов региона и его родителей.

### Метод parse_cookie {#parse_cookie}

Разбирает настройки языка интерфейса из куки my и сохраняет результат во внутренней структуре. Получить результат можно вызовом [cookie_value](#cookie_value).

```cpp
bool parse_cookie(int32_t value)
```

**Параметры**:

Параметр | Описание
----- | -----
`value` | Числовой идентификатор языка из куки my.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод cookie_value {#cookie_value}

Выдает идентификатор языка в формате куки my.

```cpp
int32_t cookie_value() const
```

**Возвращаемое значение:**

Числовой идентификатор языка в формате куки my.

### Метод parse_host {#parse_host}

Разбирает URL, по которому обратился пользователь, извлекает из него домен и сохраняет результат во внутренней структуре. Получить результат можно вызовом [top-level-domain](#top-level-domain).

```cpp
bool parse_host(std::string const &str)
```

**Параметры**:

Параметр | Описание
----- | -----
`str` | URL.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод top_level_domain {#top-level-domain}

Возвращает домен, к которому обратился пользователь.

```cpp
std::string const& top_level_domain() const
```

**Возвращаемое значение:**

Имя домена.

### Метод set_geo_regions {#set-geo-regions}

Сохраняет в классе данные о регионе пользователя.

```
bool set_geo_regions(std::list<int32_t> const &regions)
```

**Параметры**:

Параметр | Описание
----- | -----
`regions` | Регион пользователя в виде списка числовых идентификаторов региона и его родителей.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод set_pass_language {#set_pass_language}

Сохраняет в классе [значение](source-data.md) языка пользователя из Паспорта.

```cpp
bool set_pass_language(std::string const &str)
```

**Параметры**:

Параметр | Описание
----- | -----
`str` | Язык.


**Возвращаемое значение:**

Признак успешного завершения.


## Класс filter {#class-filter}

Содержит список языков сервиса.

**Методы класса:**

Метод | Описание
----- | -----
[empty](#meth-empty) | Проверяет наличие в фильтре элементов.
[allowed](#meth-allowed) | Проверяет наличие указанного языка в фильтре.
[add](#meth-add) | Добавляет перечень языков в класс.


### Метод empty {#meth-empty}

Проверяет наличие в фильтре элементов.

```cpp
bool empty() const
```

**Возвращаемое значение:**

True, если фильтр пуст.

### Метод allowed {#meth-allowed}

Проверяет наличие указанного языка в фильтре.

```cpp
bool allowed(std::string const &lang) const
```

**Параметры**:

Параметр | Описание
----- | -----
`lang` | Обозначение языка.


**Возвращаемое значение:**

True, если язык есть.

### Метод add {#meth-add}

Добавляет перечень языков в класс.

```cpp
void add(std::list<std::string> const &langs);
void add(std::string const &comma_separated_langs);
```

**Параметры**:

Параметр | Описание
----- | -----
`langs` | Перечень языков.


**Возвращаемое значение:**

Отсутствует.


## Класс domaininfo {#class-domaininfo}

Содержит информацию о домене и регионе пользователя.

**Методы класса:**

Метод | Описание
----- | -----
[parse_cookie_cr](#meth_parse_cookie_cr) | Добавить `cr`-блок куки [куки yp](https://wiki.yandex-team.ru/cookies/y#yp).
[parse_geo_regions](#meth_parse_geo_regions) | Добавить регион пользователя.
[parse_host](#meth_parse_host) | Добавить хост.
[set_geo_regions](#set_geo_regions) | Добавить регион.


### Метод parse_cookie_cr {#meth_parse_cookie_cr}

Разбирает `cr`-блок [куки yp](https://wiki.yandex-team.ru/cookies/y#yp) и сохраняет результат во внутренней структуре.

```cpp
bool parse_cookie_cr(std::string const &str);
```

**Параметры**:

Параметр | Описание
----- | -----
`str` | `cr`-блок [куки yp](https://wiki.yandex-team.ru/cookies/y#yp).


**Возвращаемое значение:**

Признак успешного завершения.

### Метод parse_geo_regions {#meth_parse_geo_regions}

Разбирает регион пользователя и сохраняет результат во внутренней структуре.

```cpp
bool parse_geo_regions(std::string const &str)
```

**Параметры**:

Параметр | Описание
----- | -----
`str` | Регион пользователя.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод parse_host {#meth_parse_host}

Разбирает URL, выделяет из него домен и сохраняет результат во внутренней структуре.

```cpp
bool parse_host(std::string const &str)
```

**Параметры**:

Параметр | Описание
----- | -----
`str` | URL


**Возвращаемое значение:**

Признак успешного завершения.

### Метод set_geo_regions {#set_geo_regions}

Сохраняет в классе данные о регионе пользователя.

```cpp
bool set_geo_regions(std::list<int32_t> const &regions)
```

**Параметры**:

Параметр | Описание
----- | -----
`regions` | Регион пользователя.


**Возвращаемое значение:**

Признак успешного завершения.


## Класс domain_filter {#class-domain_filter}

Хранит список доменов для переадресации.

**Методы класса:**

Метод | Описание
----- | -----
[empty](#empty) | Проверяет есть ли в фильтре элементы.
[allowed](#allowed) | Проверяет наличие указанного элемента в фильтре.
[add](#add) | Добавляет перечень доменов в класс.


### Метод empty {#empty}

Проверяет есть ли в фильтре элементы.

```cpp
bool empty() const
```

**Возвращаемое значение:**

True, если фильтр пуст.

### Метод allowed {#allowed}

Проверяет наличие указанного элемента в фильтре.

```cpp
bool allowed(std::string const &domain) const
```

**Параметры**:

Параметр | Описание
----- | -----
`domain` | Наименование домена.


**Возвращаемое значение:**

True - если `domain` есть в фильтре.

### Метод add {#add}

Добавляет перечень доменов в класс.

```cpp
void add(std::list<std::string> const &domain)
void add(std::string const &comma_separated_domains)
```

**Параметры**:

Параметр | Описание
----- | -----
`domain` | Наименование домена.
`comma_separated_domains` | Перечень доменов в виде строки с запятыми-разделителями.


**Возвращаемое значение:**

Отсутствует.


## Класс lookup {#class-lookup}

В классе реализован функционал библиотеки по выбору домена и региона пользователя, языка отображаемой страницы и списка релевантных пользователю языков.

Для хранения результатов работы функции [lookup::list](#list) используется тип `lang_list_type`:

```cpp
typedef std::list<langinfo> lang_list_type
```

**Методы класса:**

Метод | Описание
----- | -----
[lookup](#constructor) | Конструктор класса.
[need_swap](#need_swap) | Метод определяет изменился ли файл `lang-detect-data.txt` с момента создания объекта класса `lookup`.
[filepath](#filepath) | Возвращает путь к файлу данных `lang-detect-data.txt`.
[get_info](#get_info) | Выдает информацию о языке.
[find](#find) | Определяет язык отображения страницы.
[find_without_domain](#findWithoutDomain) | Определяет язык отображения страницы альтернативным алгоритмом, не учитывающим домен запроса.
[list](#list) | Создает список релевантных пользователю языков.
[find_domain](#find_domain) | Определяет домен пользователя.
[find_domain_ex](#find_domain_ex) | Определяет домен и регион пользователя.


### Конструктор {#constructor}

```cpp
lookup(std::string const &filepath)
```

**Параметры**:

Параметр | Описание
----- | -----
`filepath` | Путь к файлу с данными `lang-detect-data.txt`.


### Метод need_swap {#need_swap}

Метод определяет изменился ли файл `lang-detect-data.txt` с момента создания объекта класса `lookup`.

```cpp
bool need_swap() const
```

**Возвращаемое значение:**

True, если файл изменился.

{% note info %}

Если метод вернул true, т.е. файл изменился, следует создать новый объект класса `lookup`, а старый удалить.

{% endnote %}


### Метод filepath {#filepath}

Возвращает путь к файлу данных `lang-detect-data.txt`.

```cpp
std::string const& filepath() const
```

**Возвращаемое значение:**

Путь к файлу данных `lang-detect-data.txt`.

### Метод get_info {#get_info}

Выдает информацию о языке.

```cpp
bool get_info(std::string const &lang, langinfo &li) const;
bool get_info(int32_t cookie_value, langinfo &li) const;
```

**Параметры**:

Параметр | Описание
----- | -----
`lang` | Строковый идентификатор языка.
`cookie_value` | Идентификатор языка из куки my.
`li` | В этот параметр будет записана информация о языке.


**Возвращаемое значение:**

True, если запрошенный язык найден.

### Метод find {#find}

Определяет язык отображения страницы.

```cpp
bool find(userinfo const &ui, langinfo &li, filter const *f) const
bool find(userinfo const &ui, langinfo &li, filter const *f, std::string const &def_lang) const
```

**Параметры**:

Параметр | Описание
----- | -----
`ui` | Информация о пользователе.
`li` | В этот параметр будет записан результат работы функции.
`f` | Список доступных языков.
`def_lang` | Язык сервиса по умолчанию.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод find_without_domain {#findWithoutDomain}

Определяет язык отображения страницы альтернативным алгоритмом, не учитывающим домен запроса.

```cpp
bool find_without_domain(userinfo const &ui, langinfo &li, filter const *f, std::string const &def_lang) const
```

**Параметры**:

Параметр | Описание
----- | -----
`ui` | Информация о пользователе.
`l`i | В этот параметр будет записан результат работы функции.
`f` | Список доступных языков.
`def_lang` | Язык сервиса по умолчанию.


**Возвращаемое значение:**

Признак успешного завершения.

### Метод list {#list}

Создает список релевантных пользователю языков.

```cpp
bool list(userinfo const &ui, lang_list_type &langs, filter const *f) const
bool list(userinfo const &ui, lang_list_type &langs, filter const *f, std::string const &def_lang) const
```

**Параметры**:

Параметр | Описание
----- | -----
`ui` | Информация о пользователе.
`langs` | В этот параметр будет записан результат работы функции.
`f` | Список доступных языков.
`def_lang` | Язык сервиса по умолчанию.


**Возвращаемое значение:**

Признак успешного завершения.

{% note info %}

В списке языков первым (и иногда единственным) будет язык сервиса по умолчанию.

{% endnote %}


### Метод find_domain {#find_domain}

Определяет домен пользователя.

```cpp
std::string find_domain(domaininfo const &di, domain_filter const *f) const
std::string find_domain(domaininfo const &di, domain_filter const *f, bool &found) const
```

**Параметры**:

Параметр | Описание
----- | -----
`di` | Информация о домене.
`f` | Список доменов для переадресации.
`found` | В этот параметр будет записано значение **true**, если найден домен для переадресации.


**Возвращаемое значение:**

Имя домена.

### Метод find_domain_ex {#find_domain_ex}

Определяет домен и регион пользователя.

```cpp
void find_domain_ex(domaininfo const &di, domain_filter const *f, find_domain_result &result) const
```

**Параметры**:

Параметр | Описание
----- | -----
`di` | Информация о домене.
`f` | Список доменов для переадресации.
`result` | В этот параметр записывается результат работы функции.


**Возвращаемое значение:**

Отсутствует.


## Пример: {#example}

```cpp
#include <iostream>
#include "lookup.hpp"
#include "filter.hpp"
#include "userinfo.hpp"
#include "domaininfo.hpp"
#include "domain_filter.hpp"
#include "find_domain_result.hpp"

using namespace langdetect;
using namespace std;

int main()
{
    string data_file_path("/usr/share/yandex/lang_detect_data.txt");
    //string geo_regions="236,11119,40,225,10001,10000"; //Набережные Челны, Татарстан
    string geo_regions="24896,20529,20524,187,166,10001,10000"; //Где-то в Украине
    string url="http://mail.yandex.ru/neo2";
    string domain_filter_str="ua,kz,by";
    string accept_language="en-US,ru-RU";
    string passport_language="uk-Ua";
    int32_t cookie_lang=2;
    string lang_filter_str="uk,ru,be,kz,tt";
    string default_service_lang="ru-RU";
        
    domaininfo domain_i;
    if( !domain_i.parse_geo_regions(geo_regions) ) cout<<"Something wrong with regions string\n";

    if( !domain_i.parse_host(url) ){
        cout<<"Something wrong with URL\n";
    }
    
    domain_filter domain_f;
    domain_f.add(domain_filter_str);

    lookup ldetect(data_file_path);    

    if( ldetect.need_swap() ){
        //реализация логики по удалению текущего объекта класса lookup
        //и созданию нового с обновленным файлом lang_detect_data.txt
    }

    //find domain
    find_domain_result res_find_domain_ex;
    ldetect.find_domain_ex(domain_i,&domain_f,res_find_domain_ex);
    cout<<"=== Find domain\n";
    if( !res_find_domain_ex.found )    cout<<"No redirection will be used.\n";
    else cout<<"Domain or/and region has been changed.\n";
    cout<<"Domain: "<<url<<" --> "<<res_find_domain_ex.domain<<endl;
    cout<<"Region: \""<<geo_regions<<"\" --> \""<<res_find_domain_ex.content_region<<"\".\n";

    //find language
    cout<<"\n=== Find language\n";
    userinfo user_i;
    if( !user_i.parse_accept_language(accept_language) ) cout<<"userinfo::parse_accept_language() failed\n";
    if( !user_i.parse_geo_regions(geo_regions) ) cout<<"userinfo::parse_geo_regions() failed\n";
    if( !user_i.parse_cookie(cookie_lang) ) cout<<"userinfo::parse_cookie() failed\n";
    if( !user_i.parse_host(url) ) cout<<"userinfo::parse_host() failed\n";
    if( !user_i.set_pass_language(passport_language) ) cout<<"userinfo::set_pass_language() failed\n";
    
    
    filter lang_f;
    lang_f.add(lang_filter_str);
    
    langinfo lang_i;
    if( !ldetect.find(user_i,lang_i,&lang_f,default_service_lang) ) cout<<"lookup::find() failed\n";
    else{
        cout<<"Language Code: "<<lang_i.code<<"\nLanguage name: "<<lang_i.name<<"\nLanguage id(cookie): "<<lang_i.cookie_value<<endl;
    }
    
    //List relevant languages
    cout<<"\n=== Find list of relevant languages\n";
    lookup::lang_list_type langs;
    if( !ldetect.list(user_i,langs,&lang_f,default_service_lang) ) cout<<"lookup::list() failed \n";
    else{
        int i=0;
        lookup::lang_list_type::iterator langs_it;
        for( langs_it=langs.begin();langs_it!=langs.end();++langs_it ){
            if( i==0 ) cout<<"-- Default service language\n";
            else cout<<"-- "<<i<<endl;
            cout<<"Language Code: "<<(*langs_it).code<<"\nLanguage name: "<<(*langs_it).name<<"\nLanguage id(cookie): "<<(*langs_it).cookie_value<<endl;
            ++i;
        }
    }
    
    return 0;
}
```

Результат:

```no-highlight
=== Find domain
Domain or/and region has been changed.
Domain: http://mail.yandex.ru/neo2 --> mail.yandex.ua
Region: "24896,20529,20524,187,166,10001,10000" --> "24896".

=== Find language
Language Code: uk
Language name: Ua
Language id(cookie): 2

=== Find list of relevant languages
-- Default service language
Language Code: ru
Language name: Ru
Language id(cookie): 1
-- 1
Language Code: uk
Language name: Ua
Language id(cookie): 2

```

