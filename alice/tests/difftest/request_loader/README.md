# Общая информация

Перед запуском загрузчика убедитесь, что у вас на машине есть токен: https://yt.yandex-team.ru/docs/description/common/auth.html

Во время запуска можно указать таблицу, откуда брать запросы; ограничение по количеству запросов; папку, куда сохранять запросы; маркер для новых GUID (слово в начале GUID); а также фильтры.

Лучше всего собирать программу через `ya make --build=release`.

# Подготовка таблицы

Можно указать обычную таблицу, куда сохраняются логи. Например, `//home/logfeller/logs/megamind-log/1d/2020-01-13`.

В таких таблицах около 2 млрд логов, но 5000 запросов находятся за несколько минут в пределах первых нескольких миллионов записей.

Но можно подготовить временную таблицу и запустить загрузчик на ней. Для этого можно выполнить YQL-запрос, который автоматически создаст временную таблицу.
Например, этот запрос отработает 10-30 минут и создаст из таблицы в (условных) 1.7 млрд записей
временную таблицу из всего лишь (условных) 70 млн записей, где нет ничего лишнего:

```
SELECT * FROM hahn.`home/logfeller/logs/megamind-log/1d/2020-01-20`
WHERE Line == 278 AND Message LIKE '{\"app%';
```

(Запрос может поменяться со временем)

Далее, для фильтра, где нам нужны запросы от конкретных `device_id` из списка, можно сделать более сложный запрос, который также быстро отработает (5-10 минут):

```
SELECT * FROM hahn.`home/logfeller/logs/megamind-log/1d/2020-02-04`
WHERE Line == 278 AND Message LIKE '{\"app%' AND (
    Message LIKE '%74005034440c0819054e%' OR
    Message LIKE '%941050344110202b03cb%' OR
    Message LIKE '%643078918324071effcf%' OR
    Message LIKE '%74005034440c082704ce%' OR
    Message LIKE '%74005034440c0817070e%' OR
    Message LIKE '%74005034440c081d050e%' OR
    Message LIKE '%74005034440c04170b4e%' OR
    Message LIKE '%210d840066ffc9f7d6e4013278ecccd3%' OR
    Message LIKE '%210d8400798cecb5ce34053ed90f0fda%' OR
    Message LIKE '%210d8400d51fb6db4e700420df572e8e%' OR
    Message LIKE '%210d8400ebefdedb46f44402195dedba%' OR
    Message LIKE '%210d8400f8e0d407deb5031a78a80e34%' OR
    Message LIKE '%15154883664375224318%' OR
    Message LIKE '%FF98F0108AF159FD2281769C%' OR
    Message LIKE '%FF98F010A51DD23CFDA39DE0%' OR
    Message LIKE '%FF98F010C01F17055232DA65%' OR
    Message LIKE '%FF98F010F5D958642F7A4F78%' OR
    Message LIKE '%WK7Y_0000601%' OR
    Message LIKE '%WK7Y_0000606%' OR
    Message LIKE '%WK7Y_0000642%' OR
    Message LIKE '%WK7Y_0000676%' OR
    Message LIKE '%pasha-wk7y%' OR
    Message LIKE '%FF98F01A662CEAFE7346FE37%' OR
    Message LIKE '%FF98F029A02AD58215F7F78C%' OR
    Message LIKE '%FF98F029C964AD012AEC2529%' OR
    Message LIKE '%FF98F02996739FB2945E88EF%' OR
    Message LIKE '%FF98F02996F743EB1E5CA97C%' OR
    Message LIKE '%FF98F029348996C3F6577FF7%' OR
    Message LIKE '%FF98F029D6C7F4B299F1962E%'
);
```

Парсить 200 миллионов или 2 миллиардов строк на месте неадекватно, нужно самую тяжелую работу оставить на стороне YT.

# Фильтры

По умолчанию фильтра нет, принимаются все запросы, которые распарсились. Указать фильтр можно через `--filter <filter name>`.

* `simple` - допускает всё, кроме запросов с Яндекс.Станции
* `yandex-station` - допускает только запросы с Яндекс.Станции
* `beta-testers` - допускает только запросы с `device_id`, указанном в `beta_testers.txt`

# Пример использования

Пусть мы хотим скачать 10000 запросов, из них 5000 от Яндекс.Станции, от таблицы по умолчанию. Тогда можно запустить две команды:

`./loader --limit 5000 --filter yandex-station`

`./loader --limit 5000 --filter simple`

Первая команда загрузит только Яндекс.Станцию, вторая все запросы, кроме Яндекс.Станции.

# Загрузка на Sandbox

После того, как получили запросы, нужно создать ресурс `MEGAMIND_REQUESTS` на Sandbox (локальном или на проде). Пусть запросы лежат в папке `output/`. Используем `ya upload` - https://wiki.yandex-team.ru/yatool/upload/.

Загрузка на локальный Sandbox: `ya upload --do-not-remove --url=http://localhost:12121/ output/ --skynet --type MEGAMIND_REQUESTS`. Здесь подразумевается, что Sandbox поднят на порту 12121.

Загрузка на продовый Sandbox: то же самое, но url до `https://sandbox.yandex-team.ru` по умолчанию: `ya upload --do-not-remove output/ --skynet --type MEGAMIND_REQUESTS`.
