# Установка

Если права пользователей в системе сконфигурированы стандартным образом, установку следует производить от имени **пользователя с правами суперпользователя**

Установка SpeechKit Box выполняется с помощью установочного скрипта `yandex-speechkitbox-cloud-installer.sh`.
**Шаг 1. Получите скрипт**
```
$ wget https://speechkitbox.voicetech.yandex.net/speechkitbox/{{ API-key }}/yandex-speechkitbox-cloud-installer.sh
$ sudo chmod +x yandex-speechkitbox-cloud-installer.sh
```

{% note info %}

Если вы не смогли скачать скрипт из-за ошибки <q>Нет доверия сертификату для speechkitbox.voicetech.yandex.net</q>, используйте команду:

```
$ curl -LO https://speechkitbox.voicetech.yandex.net/speechkitbox/API-key/yandex-speechkitbox-cloud-installer.sh
```

{% endnote %}

**Шаг 2. Запустите скрипт**
```
$ sudo ./yandex-speechkitbox-cloud-installer.sh     
```
**Шаг 3. Запустите менеджер процессов**
```
$ sudo service supervisor start
```

Результат работы установочного скрипта:

- установлен UniMRCP-сервер;
    
- установлен менеджер процессов [Supervisor](http://supervisord.org/);
    
- cкрипт ``yandex-speechkitbox-cloud`` скопирован в ``/usr/bin/``.
    

Проверить, успешно ли выполнена установка, можно с помощью менеджера процессов (см. раздел [Мониторинг](monitoring.md)).

