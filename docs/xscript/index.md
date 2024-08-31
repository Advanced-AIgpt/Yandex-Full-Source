# Введение

**Документ является** описанием XScript 5 как языка и среды исполнения сервисов. Он включает обзор процесса обработки страниц и механизмов кэширования XScript, полный XML-синтаксис, описания всех видов блоков и их методов, а также XSL-функций (XSLT extensions).

Кроме того, документ содержит рекомендации по практическому использованию XScript, включая инструкции по созданию XML-страниц на XScript, наложению XSL-преобразований, вставке HTTP-заголовков ответа и кук, и т.д.

**Целевая аудитория документа** - разработчики интерфейсов, создающие страницы сервисов с использованием XScript 5. Кроме того, документ будет полезен системным администраторам, которых интересует структура и настройки модулей XScript.

**Документ не является** руководством по XScript 4, не содержит информации о функциональности XScript 4 и переходе с XScript 4 на XScript 5.

Документ может быть полезен CORBA-разработчикам и разработчикам XScript как источник общей информации об XScript, однако эти группы сотрудников не являются целевой аудиторией документа.

## Самое популярное {#most-popular}

#|
|| [Методы](./appendices/block-auth-methods.md) [Auth-блока](./reference/auth-block.md) | [Методы](./appendices/block-lua-other-methods.md) [Lua-блока](./reference/lua.md) | [While-блок](./concepts/block-while-ov.md) ||
|| [Методы](./appendices/block-banner-methods.md) [Banner-блока](./reference/banner-block.md) | [Методы](./appendices/block-mist-methods.md) [Mist-блока](./reference/mist.md) | [XSL-функции](./appendices/xslt-functions.md) ||
|| [Методы](./appendices/block-custom-morda-methods.md) [блока custom-morda](./reference/custom-morda.md) | [Методы](./appendices/block-mobile-methods.md) [Mobile-блока](./reference/mobile-block.md) | [Теги XScript](./reference/add-header.md) ||
|| [Методы](./appendices/block-file-methods.md) [File-блока](./reference/file.md) | [Методы](./appendices/block-tinyurl-methods.md) [Tinyurl-блока](./reference/tinyurl.md) | Теги [\<guard\>](./reference/guard.md) и [guard-not](./reference/guard-not.md) ||
|| [Методы](./appendices/block-geo-methods.md) [Geo-блока](./reference/geo.md) | [CORBA-блок](./concepts/block-corba-ov.md) | [Типы параметров методов](./appendices/block-param-types.md) ||
|| [Методы](./appendices/block-http-methods.md) [HTTP-блока](./reference/http.md) | [Local-блок](./concepts/block-local-ov.md) | [Кэширование](./concepts/caching-ov.md) ||
|#

## Структура документа {#structure}

Данный документ состоит из следующих разделов:

1. [Концепции](./concepts/xscript-ov.md): основные понятия XScript, ответы на вопросы "что это?"
1. [Процедуры](./tasks/how-to-design-page-with-xscript.md): пошаговые инструкции по решению практических задач, ответы на вопросы "как сделать?"
1. [Теги XScript](./reference/add-header.md).
1. [Методы блоков XScript](./appendices/block-auth-methods.md).
1. [Типы параметров методов](./appendices/block-param-types.md).
1. [Атрибуты блоков XScript](./appendices/attrs-ov.md).
1. [XSL-функции](./appendices/xslt-functions.md).
1. [Настройки](./appendices/config.md): описание настроек модулей, а также переменных окружения XScript.
1. [Приложения](./appendices/attrs-ov.md): более подробная инфорация о некоторых возможностях XScript (сбор статистики, способы адресации файлов); примеры.

О выпущенных версиях документа читайте в разделе [Релизы](./concepts/releases.md).

