# Как работает Case Management

{% include notitle [theory](../_includes/theory.md) %}

## Что под капотом

Case Management работает на основе AntiFraud Core. Он обращается по API к базе данных и предоставляет вам интерфейс для работы с событиями. 

События создаются по API внешними сервисами и записываются в базу данных антифрода. [Подробнее о видах событий и их формате](https://wiki.yandex-team.ru/fintech-team/fintech-antifraud/antifraud-events).

В терминах антифрода:
* Событие — это контекст, `analyzer_context`.
* Кейс или алерт — `contexts_group`. 
* Строчка с информацией о клиенте — `user_feature`.

Когда вы что-то делаете с алертом, то через API Антифрода проставляется тег:
* Статус или заметка — проставляются в `context_group`, которые соответствуют алерту.
* Действие с алертом (резолюции) — проставляется в `analyzer_context`, т.е. событиям алерта. По умолчанию тег проставляется всем событиям, но в интерфейсе можно выбрать, каким именно событиям событиями проставить тег — снять галочки.
* Действие с клиентом — специальный тег класса `generated_action_tag`, проставляется какому-то из привязанных событий (`analyzer_context`), любому.

А в бэкенде на выставление этих тегов могут быть настроены обработчики, которые и выполняют сами действия.