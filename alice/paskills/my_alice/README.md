# Локальный запуск

## Подготовка

Установить переменную окружения `$ARCADIA` в корень репозитория.

Скачать геобазу и информацию о таймзонах (сохраняет tzdata в `~/.geobase`).
```shell script
./prepare_geobase.sh
```


```shell script
ya make && ./my_alice.sh --debug --port 6900
```

`stdout` будет перенапрален в файл `my_alice.log`

## Протестировать в браузере

В аппхосте можно подменить узел графа при помощи srcrwr. Бекенд представлен тремя узлами, подменить нужно их все или хотя бы `BACKEND_PRE` и `BACKEND`:

https://hamster.yandex.ru/alice/home?graph=my_alice&srcrwr=BACKEND_PRE:ivangromov-dev.sas.yp-c.yandex.net:6900:10000000&srcrwr=BACKEND:ivangromov-dev.sas.yp-c.yandex.net:6900:10000000
