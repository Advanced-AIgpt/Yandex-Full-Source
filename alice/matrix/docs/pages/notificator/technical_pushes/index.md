# Что такое технические пуши

{% include notitle [Взаимодействие между устройствами и backend'ами](../common/_includes/interaction_between_devices_and_backends.md) %}

Как раз для этого были созданы технические пуши, они позволяют послать в устройство пользователя **почти** произвольную [SpeechKit директиву](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/speechkit/directives.proto?rev=r9437624#L14-34) (protobuf с командами для устройства) в любой момент.

## На какие устройства можно присылать технические пуши {#where_can_i_send_technical_pushes}

Технические пуши можно присылать только на колонки, откуда берется это ограничение можно прочитать [тут](https://docs.yandex-team.ru/alice-matrix/pages/notificator/connections_inverted_index/#why_only_quasar).

## Почему можно послать "почти" произвольную SpeechKit директиву, а не любую? Что лучше всего использовать? {#why_almost_any_and_what_is_the_best_way_to_use}

Нет гарантии что устройство умеет обрабатывать произвольную директиву вне активного event'а.

В данный момент гарантия корректной обработки есть только для SpeechKit директив содержащих [TTypedSemanticFrame](https://docs.yandex-team.ru/alice-scenarios/megamind/contract). Для них гарантированно, что устройство сделает запрос с этим typed semantic frame'ом (будет послан отдельный event в uniproxy).

И именно SpeechKit директивы, содержащие typed semantic frame'ы, и надо использовать.

Для преобразования typed semantic frame'ов в SpeechKit директивы есть специальное [api](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/api/utils/directives.h?rev=r8623468#L8).

## Как прислать сценарную директиву {#how_to_send_a_scenario_directive}

Никак. Без участия [megamind](https://docs.yandex-team.ru/alice-scenarios/megamind) нельзя корректно составить сценарную директиву (в частности проставить [deeplink'и](https://wiki.yandex-team.ru/alice/megamind/protocolscenarios/proto/deeplinks)).

Если вам зачем-то это нужно, обратитесь [в чат поддержки](https://docs.yandex-team.ru/alice-matrix/pages/contacts).

## Как прислать совсем произвольную SpeechKit директиву (содержащую не typed semantic frame или сценарную директиву) {#how_to_send_trash}

{% note alert %}

Скорее всего вы этого не хотите, но если вам это нужно, пожалуйста, сначала обратитесь в [в чат поддержки](https://docs.yandex-team.ru/alice-matrix/pages/contacts), а уже потом делайте, даже если "о, я попробовал(а), и оно, кажется, работает".

{% endnote %}

## Как происходит доставка технического пуша {#delivery_mechanism}

Более подробно механизм доставки описан [тут](https://docs.yandex-team.ru/alice-matrix/pages/notificator/technical_pushes/implementation).

Если коротко:
* В момент посылки запроса в notificator происходит попытка отослать технический пуш
* Если не получилось, следующие попытки будут производиться только при переподключении устройства к сети, пока не истечет ttl технического пуша

{% note warning %}

Если ttl технического пуша равен нулю, и устройство пользователя не было подключено к сети, ретраев не будет.

Также учтите что список подключений [поддерживается не идеально, и колонки в данный момент offline примерно 30 секунд из часа](https://docs.yandex-team.ru/alice-matrix/pages/notificator/connections_inverted_index/#update_lag).

{% endnote %}
