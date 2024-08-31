# Использование стабов для чего угодно - теория

Этот проект - обновленный Joker, предназначенный для хранения и получения стабов.

Он поднят на балансере `joker-light.alice.yandex.net`, крутится на нескольких небольших подах и хранит информацию о сессиях и стабах в одной большой базе YDB.

Его можно использовать как угодно в своих целях. Основной мотив использования - изолировать влияние изменчивости внешних источников при обстреле наших сервисов.

## Что такое источники и стабы?

**Источник** (внешний источник) - какой-то URL другого сервиса Яндекса, куда может обращаться наше приложение. На данный момент поддерживается схема HTTP/HTTPS.

**Стаб** - сохраненный ответ источника после обращения к нему с HTTP-реквестом.

## Что такое сессия в Joker и как её настроить?

**Сессия** - набор изолированных стабов. В YDB-базе каждой сессии отводится отдельная таблица.

Зачем это нужно? Дело в том, что мы можем хотеть разного поведения Joker со стабами. Именно сессией определяется, как Joker ведёт себя со стабами.

Каждой сессии доступно две настройки: `fetch_if_not_exists` и `imitate_delay`.

`fetch_if_not_exists` - если true, то Joker будет ходить в источник и создавать новый стаб, если нужного стаба нет. Иначе он возвратит ошибку HTTP 418.

`imitate_delay` - если true, то Joker будет задерживать возвращение стаба. Так как при создании стаба Joker сохраняет количество времени, потраченного
на обращение к внешнему источнику, то при этой настройке он ожидает это время.

Чтобы создать или обновить сессию с названием `arcadia`, достаточно выполнить в консоли:

```
curl 'http://joker-light.alice.yandex.net/session?id=arcadia'
```

По умолчанию обе настройки установлены в false. Чтобы создать или обновить сессию с обеими настройками в true, достаточно выполнить в консоли:

```
curl 'http://joker-light.alice.yandex.net/session?id=arcadia&fetch_if_not_exists=1&imitate_delay=1'
```

## Разделяет ли Joker стабы между запросами?

Когда мы обстреливаем наш сервис запросами, неплохо для каждого запроса иметь свой под-набор стабов, изолированных от всех остальных стабов.

Для каждого запроса нужно в сервисе взять его уникальный неменяющийся id. В megamind это request\_id, в других сервисах это может быть хэш-сумма от текста запроса.
Этот id нам пригодится далее.

Если сервис ходит в источники вне запросов (например, ходит куда-то по таймеру), можно использовать какой-нибудь дефолтный id, например, `default`.

## Как Joker понимает, что нужный стаб есть или что его нет?

Для каждого HTTP-реквеста, направленного в Joker, считается "хэш", зависящий только от его HTTP-метода (GET, POST) и адреса без CGI (CGI - часть адреса после вопросительного знака).

Уникальность хэша проверяется в рамках отдельной пары (id сессии, id запроса).

То есть, если для данной сессии и запроса были отправлены реквесты `http://get.yandex.ru/money?how=100` и `http://get.yandex.ru/money?how=3000&currency=dollars` 
(и с произвольными хэдерами и body), то для Joker это выглядит как один и тот же реквест.

## Из какой сети работает Joker?

Сейчас все поды работают под `_ALICEDEVNETS_`. Так что, скорее всего, почти все prod-источники будут априори недоступны, и адреса внешнего интернета тоже.
Поднимайте свой сервис на dev-окружении.

# Как сделать прокси-реквест в Joker

Пусть мы знаем id нужной сессии и id текущего запроса (или `default`, если ходим вне запроса).

Всё, что нужно сделать - поменять адрес запроса на `http://joker-light.alice.yandex.net/http` и добавить хедеры `x-host: ADDRESS`
и `x-yandex-joker: sess=SESSION_ID&test=REQUEST_ID` (в последнем хедере id сессии и id запроса).

Пример:

```
import requests

url = 'https://yandex.ru/'

# Обычный запрос
def raw_request():
    r = requests.get(url)
    print(r.status_code)
    print(r.elapsed.total_seconds())

# Запрос с прокси
def proxy_request():
    headers = {
        'x-yandex-joker': 'sess={}&test={}'.format('arcadia', 'test-id'),
        'x-host': url
    }
    r = requests.get('http://joker-light.alice.yandex.net/http', headers=headers)
    print(r.status_code)
    print(r.elapsed.total_seconds())

raw_request()
proxy_request(i)
```