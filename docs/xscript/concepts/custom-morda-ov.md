# Объект CustomMorda: проектные настройки

## Определение {#definition}

_Yandex::CustomMorda_ является одним из базовых объектов XScript и используется для хранения проектных настроек пользователя.

Интерфейс объекта описан в [custom_morda.idl](https://svn.yandex.ru/wsvn/xscript/xscript-corba/trunk/idl/custom_morda.idl).


## Особенности работы {#workdetails}

Объект предоставляет интерфейс к куке my, чтобы методам CORBA-объектов не приходилось самостоятельно с ней разбираться.


## Особенности использования {#usagedetails}

По возможности рекомендуется вместо объекта CustomMorda использовать [блок Custom-morda](block-custom-morda-ov.md). Это позволит увеличить скорость обработки страницы и снизить нагрузку на сеть.

Сервант yandex-mist обладает рядом методов для работы с CustomMorda. Доступ к данному объекту в XScript осуществляется всегда через параметр [объектного типа](parameters-complex-ov.md) CustomMorda.


## Время жизни {#lifetime}

Время жизни объекта CustomMorda соответствует времени работы над данным HTTP-запросом.

### Узнайте больше {#learn-more}
* [Custom-morda-блок](../concepts/block-custom-morda-ov.md)
* [Общий процесс обработки запроса](../concepts/request-handling-ov.md)
* [Все типы параметров методов, вызываемых в XScript-блоках](../appendices/block-param-types.md)