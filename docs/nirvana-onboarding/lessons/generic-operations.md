# Урок 8. Использование Generic-операций

{% include [note-alert](../_includes/onboarding-alert.md) %}

Generic-операции позволяют определить своё поведение с помощью кода в одной из опций. С помощью таких операций можно, например, сортировать, отбирать, объединять и проводить другие манипуляции с тысячами строк данных. Часто для обозначения таких операций используют термин **лямбда** (-выражения, -функции).

**Это урок для самостоятельного изучения.**

Рекомендуем начать с [Руководства разработчика, раздел «Операции»](https://docs.yandex-team.ru/nirvana/concepts/operations).

В популярных операциях на базе Hitman-процессора используется язык программирования **Groovy**. Вот несколько хороших ресурсов для его изучения:
1. Успешно выполненные графы коллег. Лучший способ понять, как что-то работает — посмотреть, как это работает в «боевых» условиях. Изучи входные данные, сравни с выходными данными и попробуй понять, какие именно изменения произошли в результате применения Groovy-выражения.
1. [Песочница Хитмана](https://hitman.yandex-team.ru/sandbox). [Хитман](https://hitman.yandex-team.ru) — внутренний сервис Яндекса, тесно связанный с Нирваной. В его песочнице ты можешь «безопасно» потестировать Groovy-выражения на небольшом объеме данных и посмотреть, что получится. Для тех же целей можно загрузить Groovy с [официальной страницы](http://groovy-lang.org/download.html) на свой компьютер или использовать один из онлайн-редакторов.
1. [Вики-страничка с примерами](https://wiki.yandex-team.ru/hitman/nirvana/json/groovyexamples/) Groovy-выражений.
1. Учебник [Groovy Goodness](https://docviewer.yandex-team.ru/view/1120000000166962/?page=1&*=wBN1WbSK5J7ikJ%2FJkosb26nzLRB7InVybCI6InlhLXdpa2k6Ly93aWtpLWFwaS55YW5kZXgtdGVhbS5ydS91c2Vycy9ldmdlbml5YWFuL3F1aWNrc3RhcnRuaXJ2YW5hL2dyb292eS1nb29kbmVzcy1ub3RlYm9vay5wZGYiLCJ0aXRsZSI6Imdyb292eS1nb29kbmVzcy1ub3RlYm9vay5wZGYiLCJub2lmcmFtZSI6ZmFsc2UsInVpZCI6IjExMjAwMDAwMDAxNjY5NjIiLCJ0cyI6MTY0Mzk1OTUxMjc2OCwieXUiOiI3NTA4NzkzNDgxNjQyNzYxMDczIn0%3D).
1. [Официальная документация](http://groovy-lang.org/documentation.html) Groovy.
1. [Вики-учебник](https://ru.wikibooks.org/wiki/Groovy) с базовыми элементами языка.
1. Интернет.
Кроме поиска по операциям в Нирване, часто можно найти вот [такие каталоги операций](https://wiki.yandex-team.ru/hitman/nirvana/), используемых в отделах.
