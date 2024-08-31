### Что это такое? ###

Прямо сейчас - микропрокси между графами, что варят нотифаи новой музыки, серий и подкастов, и нотификатором. Компенсирует оторванные дырки из нирваны до нотификатора путем проксирования ручки ```/subscriptions/user_list``` и создает видимость безопасности, проверяя, что таблички с созданными нотифаями лежат в нужных папках на ыте (предполагается, что права на эти папки будут не у всех).
Позже - перенесем внутрь сервиса весь код создания нотифаев, прикрутим tvm.

### Принцип работы ###
Графы в нирване идут в ручку ```/subscriptions/user_list``` (просто проксирующую запрос до [соответствующей ручки](https://wiki.yandex-team.ru/users/dolgawin/notifikacii/#polucheniespiskapodpisannyxpolzovatelejj) нотификатора), получают список пользователей и для них собирают таблички с данными для пушей, которые складывают в папки (в конфиге ```app.yt_dirs.<push_type>``` ), после чего сообщают сервису о готовности путем похода в ручку ```/process_push/<push_type>/<table_name>```.

Далее сервис проверяет, действительно ли таблица лежит в соответствующей ей папке, после чего формирует пуши, отправляет их в нотификатор, результат записывает в другую таблицу (в конфиге ```app.yt_dirs.<push_type>_result``` ) и удаляет исходную.

Получение адреса хоста (т.к. внутри няни может произвольно меняться под, позже, возможно, заведем балансер): 
```
curl "http://sd.yandex.net:8080/resolve_endpoints/json" --data '{"cluster_name": "sas", "endpoint_set_id": "mediaalice-notifier-proxy", "client_name": "rabidokov"}'
```
Пример запроса в ручку ```process_push```:
```
curl -X POST -H 'Authorization: OAuth <yt_token>' 'http://mediaalice-notifier-proxy-1.sas.yp-c.yandex.net:80/process_push/music/notifications_2022-04-18'
```

### Где что лежит ###
Процессы в хитмане, собирающие данные для пушей:
* [music_push](https://hitman.yandex-team.ru/projects/media_alice/music-push/?jobCreationTimeQuery=YEAR&jobPage=1&pageSize=20)
* [series_push](https://hitman.yandex-team.ru/projects/media_alice/series-push/?jobCreationTimeQuery=YEAR&jobPage=1&pageSize=20)
* [podcasts_push](https://hitman.yandex-team.ru/projects/media_alice/podcasts-push/?jobCreationTimeQuery=YEAR&jobPage=1&pageSize=20)

[Сервис в няне](https://nanny.yandex-team.ru/ui/#/services/catalog/mediaalice_notifier_proxy)

Релиз - собрать бинарь руками/таской в сендбоксе, зайти в сервис в няне и поменять бинарник

Правка конфига - зайти в сервис в няне и руками поправить файл конфига (можно и напрямую на хосте, но если он вдруг переедет на другой под, конфиг сбросится до того, что указан в няне)
