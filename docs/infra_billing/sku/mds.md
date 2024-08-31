# SKU MDS

{% note info "Примечание" %}

Раздел находится в разработке.

{% endnote %}

В "большой" MDS входит три сервиса: MDS, S3, Avatars.

Основные группы предоставляемых ресурсов:
- квота на HDD/SSD
- запросы в API
- междц и внешний трафик (трафик внутри ДЦ не тарифицируется)

Список SKU с единицами измерения и описанием приведён в таблице 1.

<small>Таблица 1 — Перечень SKU</small>

| SKU в биллинге | Единица измерения | Описание |
| :--- | ---- | ---- |
| `<service>.quota_space.<class>` | Гибибайт | Квота на суммарный объём данных в неймспейсах (MDS, Avatars) или бакетах (S3) сервиса.<br>`<service>` - сервис,  в котором заказана квота: MDS, S3, Avatars<br>`<class>` - класс хранения, который включает в себя тип диска (hdd, ssd) и коэффициент репликации (lrc, x2, x3, x4, x5):<br>`s3.quota_space.hdd.x3`<br>`mds.quota_space.hdd.lrc`<br>`avatars.quota_space.hdd.x4` |
| `<service>.api.get.<class>`<br>`<service>.api.head.<class>`<br>`<service>.api.options.<class>`<br> | Штуки | Количество читающих запросов в API (GET, HEAD, OPTIONS) |
| `<service>.api.put.<class>`<br>`<service>.api.patch.<class>`<br>`<service>.api.post.<class>`<br>`<service>.api.delete.<class>`<br> | Штуки | Количество модифицирующих запросов в API (PUT, PATCH, POST, DELETE) |
| `<service>.api.network.crossdc.RU-CENTRAL` | Гибибайт | Суммарный трафик сервиса между датацентрами в России |
| `<service>.api.network.crossdc.RU-CENTRAL_FIN` | Гибибайт | Суммарный трафик сервиса между датацентрами в России и Финляндии |
| `<service>.api.network.external-dc-geo.ingress` | Гибибайт | Суммарный входящий трафик из интернета |
| `<service>.api.network.external-dc-geo.egress` | Гибибайт | Суммарный исходящий трафик в интернет |

