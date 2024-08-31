//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind](../index.md)/[BindCardMediator](index.md)

# BindCardMediator

[datasource]\
open class [BindCardMediator](index.md) : [CardInputMediator](../-card-input-mediator/index.md)

Медиатор для привязки карт.

## Constructors

| | |
|---|---|
| [BindCardMediator](-bind-card-mediator.md) | [datasource]<br>fun [BindCardMediator](-bind-card-mediator.md)() |

## Functions

| Name | Summary |
|---|---|
| [cancel](cancel.md) | [datasource]<br>open override fun [cancel](cancel.md)()<br>Отменить запущенный после нажатия кнопки процесс. |
| [process](process.md) | [datasource]<br>@UiThread()<br>open override fun [process](process.md)()<br>Обработать нажатие кнопки. |
| [reset](reset.md) | [datasource]<br>open override fun [reset](reset.md)()<br>Отсоединиться от всего UI и выполнить сброс. |
| [setBindApi](set-bind-api.md) | [datasource]<br>fun [setBindApi](set-bind-api.md)(api: [BindApi](../../com.yandex.payment.sdk.datasource.bind.interfaces/-bind-api/index.md))<br>Задать api привязок. |

## Inheritors

| Name |
|---|
| [BindCardMediatorLive](../-bind-card-mediator-live/index.md) |
