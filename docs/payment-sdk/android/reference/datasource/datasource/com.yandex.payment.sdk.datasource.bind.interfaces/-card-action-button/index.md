//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind.interfaces](../index.md)/[CardActionButton](index.md)

# CardActionButton

[datasource]\
interface [CardActionButton](index.md)

Интерфейс основной кнопки действия.

## Types

| Name | Summary |
|---|---|
| [State](-state/index.md) | [datasource]<br>sealed class [State](-state/index.md) |

## Functions

| Name | Summary |
|---|---|
| [setOnClick](set-on-click.md) | [datasource]<br>abstract fun [setOnClick](set-on-click.md)(callback: () -> Unit)<br>Установить листенер на нажатие. |
| [setState](set-state.md) | [datasource]<br>abstract fun [setState](set-state.md)(state: [CardActionButton.State](-state/index.md))<br>Установить новое состояние кнопки. |
