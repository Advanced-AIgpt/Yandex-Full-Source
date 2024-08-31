# Часто задаваемые вопросы

## Как посмотреть то, что приходит в данных?

* `&json_dump_responses=1` - выводит все, что приходит с бека
* `&json_dump_responses=YDO_BACKEND` - вершина бека
Все эти параметры не работают на локальных бетах, можно на бете мастера смотреть

## Как посмотреть логи бекенда

Их можно получить например курлом, прокинув параметры `&json_dump_responses=YDO_BACKEND&json_dump_responses=YDO_BACKEND_REQUEST_EXTRACTOR`. Пример:

```bash
curl 'https://uslugi.yandex.ru/api/to_next_order_clarifier_step?call_center=1&nonce=2&json_dump_responses=YDO_BACKEND&json_dump_responses=YDO_BACKEND_REQUEST_EXTRACTOR'
```

Есть подробная [дока](https://wiki.yandex-team.ru/ydo/backend/infra/debuginfoforbackend/) как собрать отладочную информацию для бэкенда

## Как узнать reqid

Прокинуть параметры `&json_dump_responses=YDO_TEMPLATES`, в ответе будет поле reqid

## Swagger

[Swagger](https://renderer-ydo-dev-master.hamster.yandex.ru/uslugi/swagger/index.html) позволяет узнать какие параметры есть в ручке, какие обязательны/необязательны. Также можно тестово дергать ручки тестировщикам/разработчикам/всем заинтересованным для создания нужных состояний

## Если бэкенд отвечает как-то странно и вы не понимаете почему

Можете самостоятельно подглядеть в логи и попробовать по ошибкам в них понять, что же не так с исходным запросом. Для этого нужно узнать reqid запроса и посмотреть логи в [setrace](https://setrace.yandex-team.ru/web/search)

## Тестирование беты на проде

1. Заходим на [stoker](https://stoker.z.yandex-team.ru/records)
2. В поле ввода вводим номер ПРа и переходим запись в стокере ([пример](https://stoker.z.yandex-team.ru/TAGGED/renderer-ydo-pull-2741918))
3. Переходим на вкладку [rewrite](https://stoker.z.yandex-team.ru/TAGGED/renderer-ydo-pull-2741918/rewrite)
4. Нажимаем на карандаш
5. От туда копируем строку для YDO_TEMPLATES
6. Добавляем к урлу параметр `srcrwr` (пример: `srcrwr=YDO_TEMPLATES:renderer-betas-8-4.iva.yp-c.yandex.net:4040:5000;renderer-betas-replica-9-2.sas.yp-c.yandex.net:4080:5000`)
