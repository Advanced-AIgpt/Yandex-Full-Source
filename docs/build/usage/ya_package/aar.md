# Сборка aar-пакетов

Для сборки aar-пакетов, нужно выполнить
```bash
ya package --aar <package.json>
```

## Публикация пакетов { #publish }
Для публикации aar-пакетов, нужно добавить к запуску ключ ```--publish-to <settings.xml>```, `settings.xml` должен содержать в себе параметры, необходимые для загрузки пакета в `artifactory`.
Пример такого settings-файла можно найти по [ссылке](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/package/tests/create_aar/good_settings.xml?rev=7217997&blame=true).
При публикации пакета внутри ```ya package``` вызывается команда ```mvn -s <settings.xml> deploy:deploy-file -Dfile=<путь до собранного aar пакета>```.
