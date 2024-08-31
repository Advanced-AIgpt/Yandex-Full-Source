# Переменные окружения веб-сервера, обрабатываемые XScript-ом

{% note info %}

Данный раздел предназначен в первую очередь для системных администраторов.

{% endnote %}


В приведенной ниже таблице описаны переменные окружения веб-сервера, обрабатываемые XScript-ом.

#|
|| Библиотека | Параметр | Тип/ допустимые значения | Описание ||
|| libxscript | SHOW_ELAPSED_TIME | yes, no, true, false, 1, 0 | Если данной переменной присвоено значение "yes", "true" или "1", в development mode в XML-файле, выдаваемом пользователю, будут показываться времена выполнения блоков вне зависимости от значения атрибута блока [elapsed-time](attrs-ov.md#elapsed-time). ||
|| xscript-yandex | REDIRECT_BASE_URL | Строка | Переопределение параметра [redirect-base-url](config-params.md#redirect-base-url). ||
|| xscript-yandex | DEFAULT_KEY_NO | Целое число | Переопределение параметра [default-key-no](config-params.md#default-key-no). ||
|| xscript-yandex | NEED_YANDEXUID | yes, no, true, false, 1, 0 | Переопределение параметра [need-yandexuid-cookie](config-params.md#need-yandexuid-cookie). ||
|| xscript-yandex | DPS_ROOT | Строка | Переопределение параметра [dps-root](config-params.md#dps-root). ||
|| xscript-yandex |  BLACKBOX_SLEEP_TIMEOUT | Целое число | Переопределение параметра [blackbox-sleep-timeout](config-params.md#blackbox-sleep-timeout). ||
|| xscript-yandex |  BLACKBOX_RETRIES | Целое число | Переопределение параметра [blackbox-retries](config-params.md#blackbox-retries). ||
|| xscript-yandex |  PROJECT_NAME | Строка | Имя проекта, которое используется при редиректе на паспорт для мимикрии (изменение внешнего вида страницы, в зависимости от того, с какого сервиса пришел посетитель). ||
|| xscript-yandex |  OUTPUT_ENCODING | Строка | Переопределение параметра [output-encoding](config-params.md#output-encoding). ||
|| xscript-yandex |  ACCEPT_X_REAL_IP | yes, no, true, false, 1, 0 | Если данной переменной присвоено значение "yes", "true" или "1", XScript будет принимать HTTP-заголовок _X-Real-IP_. ||
|| xscript-yandex |  ACCEPT_X_ORIGINAL_HOST | yes, no, true, false, 1, 0 | Если данной переменной присвоено значение "yes", "true" или "1", XScript будет принимать HTTP-заголовок _X-Original-Host_. ||
|| xscript-corba | DONT_USE_REMOTE_CALL | yes, no, true, false, 1, 0 | Если данной переменной присвоено значение "yes", "true" или "1", удаленные вызовы выполняться не будут. ||
|#

### Узнайте больше {#learn-more}
* [ребования для системного администратора](../concepts/requirements-admin.md)
* [Файл настроек](../appendices/config.md)
* [Параметры файла настроек](../appendices/config-params.md)