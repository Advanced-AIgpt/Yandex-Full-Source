# SKU YT

{% note info "Примечание" %}

Раздел находится в разработке.

{% endnote %}

В данном разделе описаны продукты, предоставляемые провайдером YT. Описываются принципы формирования продуктов.

Все продукты YT можно условно разделить на четыре группы: вычислительные (compute), хранение (disk_space), динамические таблицы (dynamic tables) и GPU.

Список SKU с единицами измерения и описанием приведён в таблице 1.

<small>Таблица 1 — Перечень SKU</small>

| SKU в биллинге | Единица измерения | Описание |
| :--- | ---- | ---- |
| compute.integral_guarantee.burst.cpu | Ядро | Гарантия в моменте ("пиковая гарантия"), позволяет получать фиксированную гарантию в течение промежутка времени. Например, получать 2000 ядер в течение двух часов в сутки. [Подробное описание](https://docs.yandex-team.ru/yt/description/mr/scheduler/integral_guarantees). |
| compute.integral_guarantee.relaxed.cpu | Ядро | Данный вид гарантии позволяет получить фиксированный объём ресурса в среднем за сутки. [Подробное описание](https://docs.yandex-team.ru/yt/description/mr/scheduler/integral_guarantees). |
| compute.strong_guarantee.cpu | Ядро | Строгая гарантия, выдаётся по запросу, может использоваться бесконечно долго. |
| compute.usage.cpu | Ядро | Использование CPU вне зависимости от наличия гарантии. |
| compute.usage.memory | Гигабайты | Использование оперативной памяти в MapReduce операциях. |
| disk_space.hdd | Гигабайты | Дисковая квота на SATA HDD носителях. |
| disk_space.ssd | Гигабайты | Дисковая квота на SATA SSD и NVMe SSD носителях. |
| dynamic_tables.tablet_cell | Штуки | Выделенный ресурс для обслуживания запросов к динамическим таблицам. [Подробное описание](https://yt.yandex-team.ru/docs/description/dynamic_tables/dynamic_tables_overview). |
| gpu.geforce_1080ti.strong_guarantee.gpu | Карта | Строгая гарантия на GPU карту geforce_1080ti. |
| gpu.geforce_1080ti.usage.gpu | Карта | Использование GPU карты geforce_1080ti вне зависимости от наличия гарантии. |
| gpu.tesla_a100.strong_guarantee.gpu | Карта | Строгая гарантия на GPU карту tesla_a100. |
| gpu.tesla_a100.usage.gpu | Карта | Использование GPU карты tesla_a100 вне зависимости от наличия гарантии. |
| gpu.tesla_k40.strong_guarantee.gpu | Карта | Строгая гарантия на GPU карту tesla_k40. |
| gpu.tesla_k40.usage.gpu | Карта | Использование GPU карты tesla_k40 вне зависимости от наличия гарантии. |
| gpu.tesla_m40.strong_guarantee.gpu | Карта | Строгая гарантия на GPU карту tesla_m40. |
| gpu.tesla_m40.usage.gpu | Карта | Использование GPU карты tesla_m40 вне зависимости от наличия гарантии. |
| gpu.tesla_p40.strong_guarantee.gpu | Карта | Строгая гарантия на GPU карту tesla_p40. |
| gpu.tesla_p40.usage.gpu | Карта | Использование GPU карты tesla_p40 вне зависимости от наличия гарантии. |
| gpu.tesla_v100.strong_guarantee.gpu | Карта | Строгая гарантия на GPU карту tesla_v100. |
| gpu.tesla_v100.usage.gpu | Карта | Использование GPU карты tesla_v100 вне зависимости от наличия гарантии. |

## Группы кластеров

Все ресурсы YT разделены на несколько независимых кластеров. Подробнее о делении на кластеры можно прочитать в разделе [Кластеры](https://yt.yandex-team.ru/docs/overview/clusters) основной документации по YT. 

Тарифы на продукты сформулированы для групп кластеров, один и тот же продукт на каждом кластере соответствующей группы имеет одинаковую цену.

Кластеры YT сгруппированы по решаемым задачам:

- большие кластеры (big clusters):
  - hahn;
  - arnold;
  - freud;
  - hume;
- кластеры для динамических таблиц (dynamic tables):
  - seneca-sas;
  - seneca-man;
  - seneca-vla;
  - markov;
  - zeno;
- кластеры Антифрода (xurma):
  - bohr;
  - landau;
- кластеры статистики:
  - vanga;
  - pythia.

Например, следующие продукты имеют одинаковую стоимость:

- yt.hahn.compute.strong_guarantee.cpu;
- yt.arnold.compute.strong_guarantee.cpu;
- yt.freud.compute.strong_guarantee.cpu;
- yt.hume.compute.strong_guarantee.cpu;

## Особенности распределения стоимости

- 70% стоимости ядра покрывается гарантией, 30% покрывается платой за использование (аналогично для GPU карты);
- 80% стоимости *гарантированного ядра* покрывается стоимостью relaxed гарантии (flow), 20% покрывается стоимостью burst гарантии.

