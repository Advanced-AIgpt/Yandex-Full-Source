# Системные сущности NLU

В этом разделе детально рассмотрены системные сущности NLU, на какие виды запросов они матчатся и в каком формате выдают ответ.
Дополнительные примеры по всем категориям может найти в [списках тестов](https://a.yandex-team.ru/arc_vcs/alice/nlu/data/ru/test/fst_entities/canondata/).

Список всех типов есть [тут](https://docs.yandex-team.ru/alice-scenarios/nlu/granet/syntax#named-entity-types).

## sys.date, sys.time, sys.datetime

Объекты типа sys.date, sys.time и sys.datetime могут выдавать следующие варианты ответов:

##### Абсолютное значение даты и/или времени

Могут быть следующие поля:

* "years":число. Содержит значение в формате "years":2021. Внимание! При использовании выражений вида 'два года' преобразует это в 2002!
* "months":число. 1 - январь, 12 - декабрь
* "days":число. От 1 до 31.
* "hours":число. От 0 до 23 или от 0 до 11, если задан period
* "period":"am" или "period":"pm".
* "minutes":число. От 0 до 59.
* "seconds":число. От 0 до 59.

В объекте sys.date используются поля years/months/days.
В объекте sys.time - hours, minutes и seconds, плюс опционально period - он появлется, если есть явная отсылка к времени суток AM/PM.
Объект sys.datetime является комбинацией sys.date и sys.time.

Любое из полей может отсутствовать, см примеры ниже:

| Ключ       | Допустимые значения | Пример фразы                              | Итоговый JSON                         |
| -----------|:------------------- |:----------------------------------------- |:------------------------------------- |
| years      | (int)               | фильмы 'две тысячи двадцать первого года' | `{"years":2021}`                      |
|            |                     | а в 'двадцатом году' когда                | `{"years":2020}`                      |
| month      | (int) 1...12        | ласковый 'май'                            | `{"months":5}`                        |
| days       | (int) 1...31        | шуфутинского включи '3 сентября'          | `{"days":3,"months":9}`               |
|            |                     | '8 марта 22 года' что за день недели      | `{"days":8,"months":3, "years":2022}` |
| hours      | (int) 1...12 или 23 | алиса может 'три часа'                    | `{"hours":3}`                         |
| period     | (string) am/pm      | позвонишь в 'два часа ночи' мне           | `{"hours":2,"period":"am"}`           |
| minutes    | (int) 0...59        | поставь будильник на '17 0 0'             | `{"hours":17,"minutes":0}`            |
| seconds    | (int) 0...59        | 'полторы минуты'                          | `{"minutes":1,"seconds":30}`          |


##### Абсолютное значение дня недели

* Поле "weekday":число (от 1 - понедельник до 7 - воскресенье)

| Ключ          | Допустимые значения | Пример фразы | Итоговый JSON   |
| ------------- |:------------------- |:------------ |:--------------- |
| weekday       | (int) 1...7         | вторник      | `{"weekday":2}` |
|               |                     | в субботу    | `{"weekday":6}` |


##### Относительное значение даты и/или времени

Содержит 2 поля, числовой счетчик (может быть как положительным, так и отрицательным) и флаг relative запроса.

* "seconds_relative":true, "seconds":число
* "minutes_relative":true, "minutes":число
* "hours_relative":true, "hours":число
* "days_relative":true, "days":число
* "months_relative":true, "months":число
* "years_relative":true, "years":число
* "weeks_relative":true, "weeks":число

| Ключ                      | Допустимые значения | Пример фразы                              | Итоговый JSON                                                                   |
| --------------------------|:------------------- |:----------------------------------------- |:------------------------------------------------------------------------------- |
| seconds_relative, seconds | (bool) (int)        | на 'пятнадцать секунд назад'              | `{"seconds":-15,"seconds_relative":true}`                                       |
| minutes_relative, minutes | (bool) (int)        | выключись 'через 5 минут'                 | `{"minutes":5,"minutes_relative":true}`                                         |
|                           |                     | перемотай на 'полторы минуты назад'       | `{"minutes":-1,"minutes_relative":true,"seconds":-30,"seconds_relative":true}`  |
| hours_relative, hours     | (bool) (int)        | напомни мне 'через час'                   | `{"hours":1,"hours_relative":true}`                                             |


## sys.weekdays

* "repeat" - bool, содержит true или false в зависимости от признака повторения
* "weekdays" - массив int values (1 - понедельник, 7 - воскресенье)


| Ключ          | Допустимые значения | Пример фразы                 | Итоговый JSON                                |
| ------------- |:------------------- |:---------------------------- |:-------------------------------------------- |
| repeat        | (bool)              | погода 'на выходные'         | `{"repeat":false,"weekdays":[6,7]}`          |
| weekday       | [int] 1...7         | какая погода 'на вторник'    | `{"repeat":false,"weekdays":[2]}`            |
|               |                     | можно я куплю 'в пятницу'    | `{"repeat":false,"weekdays":[5]}`            |
|               |                     | предложение на 'каждый день' | `{"repeat":true,"weekdays":[1,2,3,4,5,6,7]}` |
|               |                     | я рисую 'каждую среду'       | `{"repeat":true,"weekdays":[3]}`             |


## sys.geo

По другим сущностям документация в процессе подготовки.