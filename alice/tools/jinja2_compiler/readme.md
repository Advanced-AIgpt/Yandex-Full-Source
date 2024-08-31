# Jinja2_compiler

## Что это?

jinja2_compiler - это инструмент для преобразования .proto файлов в сложные исходные файлы на любом языке программирования при помощи языка темплейтной разметки [Jinja2](https://jinja2docs.readthedocs.io/en/stable/).

При помощи этого компилятора и вспомогательных темплейтов можно автоматически построить код (а также json и другие данные) практически любой степени сложности.

Использование jinja2_compiler (также доступно по jinja2_compiler --help):

`jinja2_compiler --proto=protofile --input=templatefile --output=outputfile --message=proto_message --include=path`

Где:
* `protofile` - ссылка на протобаф, который требуется распарсить.
* `templatefile` - ссылка на файл с шаблоном кодогенерации
* `outputfile` - имя и местоположение выходного файла данных
* `--message="classname"` - название класса (message) в протобафе, который требуется для генерации ответа
* `--include=path` - дополнительные include каталоги для поиска протобафов и файлов темплейтов
* `--help` - пока справочной информации

Все опции могут повторяться больше одного раза

Количество опций --input и --output должно совпадать друг с другом, если используется больше одного вывода, то компилятор создаст сразу несколько выходных файлов, например:

`jinja2_compiler --input=file1.jinja2 --output=file1.cpp --input=file2.jinja2 --output=file2.cpp --input=file3.jinja2 --output=file3.h`

## Быстрый старт

В папке examples приведен пример компиляции протобафа request.proto (это копия части протобафа из megaming/protos/scenarios). Запустите run.sh, в папке будет создан файл данных interfaces.h. При его генерации используется шаблон tinterfaces.jinja2.

Для правильной генерации h файла из темплейта необходимо задать дополнительные опции сообщений, их можно найти в proto/options.proto:

* feature - строковое название фичи, которое приходит с клиента
* feature_type - код, который требуется сгенерировать в выходном файле:
  * SimpleSupport - кодогенерация ограничится строкой `return SupportedFeatures.contains("field_name");`
  * SupportUnsupportTrue и SupportUnsupportFalse - кодогенератор запишет следующий текст 
```
if (SupportedFeatures.contains("field_name")) {
     return true;
}                                                              
if (UnsupportedFeatures.contains("field_name")) {
    return false;
}
return false (SupportUnsupportFalse) или return true (SupportUnsupportTrue)
```
  * CustomCode - функция останется pure virtual, ей требуется собственная имплементация

## Доступные переменные внутри темплейтов

Все message, которые были спроцессированы из proto файлов и задекларированы в командной строке через опцию `--message="classname"`, доступны в темплейте по своему имени.

Пример из каталога /example:

В командной строке: `... --message=TInterfacesExample`

В jinja2 темплейте: `{%- for field in TInterfacesExample -%}`

### Поля объекта config

Глобальная переменная config содержит все входные параметры, переденные в компилятор

| Поле          | Тип               | Описание                 |
| ------------- |:----------------- |:------------------------ |
| input         | string[]          | Список входных файлов template, переданных через --input |
| output        | string[]          | Список выходных файлов, переданных через --output |
| message       | string[]          | Список классов прообафа, переданных через --message |
| proto         | string[]          | Список имен входных протофайлов, переданных через --proto |
| include       | string[]          | Список путей, переданных через --include |


### Поля объекта system

Объект system содержит записи о текущем состоянии системы

| Поле          | Тип               | Описание                  |
| ------------- |:----------------- |:------------------------- |
| user          | string            | Имя текущего пользователя |
| date          | string            | Дата компиляции в формате YYYY-MM-DD |
| time          | string            | Время компиляции в формате hh:mm:ss |
| version       | string            | Номер версии генератора |

### Поля объекта типа google protobuf Message

| Поле          | Тип               | Описание                  |
| ------------- |:----------------- |:------------------------- |
| fields        | MessageField[]    | Список полей класса       |
| options       | KeyValue[]        | Список опций класса в формате key:value |


### Поля объекта типа google protobuf MessageField

| Поле          | Тип               | Описание                  |
| ------------- |:----------------- |:------------------------- |
| name          | string            | Название поля             |
| name_lowercase  | string          | Название поля (lower_case)            |
| name_lcamelcase | string          | Название поля (lowerCamelCase)            |
| name_ucamelcase | string          | Название поля (UpperCamelCase)            |
| name_uppercase  | string          | Название поля (UPPER_CASE)            |
| type          | string            | Тип поля в нотации proto  |
| type_cpp      | subclass field::type_cpp         | Тип поля в нотации C++. Требует доступа к дополнительным полям |
| number        | int               | Номер поля в протобафе    |
| is_required   | bool  | true, если поле с флагом required         |
| is_optional   | bool  | true, если поле с флагом optional         |
| is_repeated   | bool  | true, если поле с флагом repeated         |
| is_simple     | bool  | true, если тип является простым (int, bool, ...) |
| is_complex    | bool  | обратная функция к is_simple |
| comments      | string[] | массив строк с комментариями к этому полю |
| options       | KeyValue[] | опции поля, например: feature, feature_type, ...|
| enum          | subclass[] | только для полей типа enum. Список значений enum |
| message       | protobuf Message | только для полей типа message. Ссылка на описание вложенного класса |
| oneof         | subclass field::oneof | только для полей типа oneof. Ссылка на описание переменных, которые находятся внутри этого oneof |

Замечание: is_optional выставляется в true только для полей с флагом optional (Protobuf 3). Для перечислений, которые отмечены в блоке oneof, используется следующая логика:

* Формируется "фейковая" структура типа `union`
* Все поля, принадлежащие этому oneof, записываются как члены этой внутренней структуры

### Поля объекта типа field::enum

| Поле          | Тип               | Описание                  |
| ------------- |:----------------- |:------------------------- |
| name          | string            | Название поля enum |
| value         | int               | Значение поля enum |

### Поля объекта типа field::type_cpp

| Поле          | Тип               | Описание                  |
| ------------- |:----------------- |:------------------------- |
| base          | string            | Базовое название поля (int, bool, название enum или класса) |
| full          | string            | Название поля с учетом флагов. Флаг repeated добавляет TVector<>. Флаг optional добавляет TMaybe<>. В отсутствии модификаторов значение type_cpp::full совпадает с type_cpp::simple |

### Поля объекта типа field::oneof

| Поле          | Тип               | Описание                  |
| ------------- |:----------------- |:------------------------- |
| fields        | MessageField[]    | Список полей внутри oneof |
