package ru.yandex.alice.kronstadt.core.directive

import com.yandex.div.dsl.context.CardWithTemplates
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective.Layer.Companion.DIALOG

data class ShowViewDirective constructor(
    val layer: Layer,
    val card: Card,
    val doNotShowCloseButton: Boolean = true,
    val inactivityTimeout: InactivityTimeout = InactivityTimeout.INFINITY,
    val actionSpaceId: String? = null,
) : MegaMindDirective {
    constructor(div2Card: CardWithTemplates, actionSpaceId: String, doNotShowCloseButton: Boolean)
        : this(
        card = Div2Card(div2Card), doNotShowCloseButton = doNotShowCloseButton, actionSpaceId = actionSpaceId,
        layer = DIALOG
    )

    constructor(cardId: String, actionSpaceId: String, doNotShowCloseButton: Boolean)
        : this(
        card = CardId(cardId), doNotShowCloseButton = doNotShowCloseButton, actionSpaceId = actionSpaceId,
        layer = DIALOG
    )

    constructor(
        cardId: String,
        actionSpaceId: String,
        doNotShowCloseButton: Boolean,
        inactivityTimeout: InactivityTimeout
    )
        : this(
        card = CardId(cardId), doNotShowCloseButton = doNotShowCloseButton, actionSpaceId = actionSpaceId,
        layer = DIALOG, inactivityTimeout = inactivityTimeout
    )

    constructor(cardId: String, actionSpaceId: String, doNotShowCloseButton: Boolean, layer: Layer)
        : this(card = CardId(cardId), doNotShowCloseButton = doNotShowCloseButton, actionSpaceId = actionSpaceId,
        layer = layer)

    sealed class Layer {
        companion object {
            val DIALOG = Dialog(DialogLayer())
            val CONTENT = Content(ContentLayer())
            val ALARM = Alarm(AlarmLayer())
        }
    }

    data class Content(val content: ContentLayer) : Layer()

    class ContentLayer

    data class Dialog(val dialog: DialogLayer) : Layer()

    class DialogLayer

    data class Alarm(val content: AlarmLayer) : Layer()

    class AlarmLayer

    sealed class Card

    data class Div2Card(val templateDivCard: CardWithTemplates) : Card()

    data class CardId(val id: String) : Card()

    enum class InactivityTimeout {
        SHORT,
        MEDIUM,
        LONG,
        INFINITY,
    }
}
