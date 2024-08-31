# Примеры использования

{% note warning "Внимание" %}

В wonderlogs не фильтруются абсолютно никакие логи, поэтому чтобы получить то же множество запросов, что и в dialogs, нужно прописать данные [фильтры](https://a.yandex-team.ru/arc/trunk/arcadia/alice/wonderlogs/daily/lib/dialogs.cpp?rev=r8273562#L167-178).

Например, для YQL
```
WHERE
        speechkit_request IS NOT NULL
    AND
        speechkit_response IS NOT NULL
    AND
        NOT spotter.false_activation
```

{% endnote %}

## C++
* [Маппинг](https://a.yandex-team.ru/arc/trunk/arcadia/alice/wonderlogs/daily/lib/dialogs.cpp) [wonderlogs](https://yt.yandex-team.ru/hahn/navigation?path=//home/alice/wonder/logs) в [dialogs](https://yt.yandex-team.ru/hahn/navigation?path=//home/voice/vins/logs/dialogs)

## YQL
* [Запрос](https://yql.yandex-team.ru/Operations/YCuS0QuEI31eQa6D3RVLqsGEdiKt_qGvvtQazpDPyww=), определяющий каких сценариях на колонках есть карточки и директивы одновременно
* [Запрос](https://yql.yandex-team.ru/Operations/YBvMeQPTTi92mra3Wmncig5jU50hXHsK91gI-9M-HS4=), определяющий что ещё ходит в VINS
* [Запрос](https://yql.yandex-team.ru/Operations/YCr7MPMBw4rPTPCrWLRUB3urYYP25J5OI3bqssuM4PE=), определяющий запросы под экспериментом и определённым продуктовым сценарием, в которых Алиса отвечала ошибками
* [Запрос](https://yql.yandex-team.ru/Operations/YBviR9K3DPOELS3tJtFH_QiVaCWMfBAR8wMvdJIb_CM=), определяющий запросы (с примерами request_id) в Алису с конкретным продуктовыми и фактическими сценариями по тексту запроса
* [Запрос](https://yql.yandex-team.ru/Operations/YCOs_FJ2-XwpOS4MmZ3qcs1yc_KgGzEZKvg_Z48X5sM=), определяющий запросы где определённый сценарий работал дольше порога
