# Мониторинг

Мониторинг компонентов выполняется менеджером процессов [Supervisor](http://supervisord.org/).

Запустить менеджер процессов можно командой:

```
$ sudo service supervisor start
```

Управлять менеджером процессов можно через консольную утилиту `supervisorctl`.

Чтобы получить список всех сервисов _SpeechKit Box_ и узнать их статус, выполните команду:

```
$ sudo supervisorctl status

yandex-mrcp-server               RUNNING   pid 14691, uptime 18:57:40
yandex-services-monitor          RUNNING   pid 14689, uptime 18:56:55
```

Мониторинг выполняет сервис мониторинга _yandex-services-monitor_, поэтому он должен быть запущен (то есть иметь статус _RUNNING_).

Если сервис мониторинга не запущен, запустите его следующим образом:

```
$ sudo supervisorctl start yandex-services-monitor
```

Когда менеджер процессов запущен, для управления SpeechKit Box можно использовать команду `yandex-speechkitbox-cloud` со следующими параметрами:

```
$ sudo yandex-speechkitbox-cloud --stop
$ sudo yandex-speechkitbox-cloud --start
$ sudo yandex-speechkitbox-cloud --restart
```

