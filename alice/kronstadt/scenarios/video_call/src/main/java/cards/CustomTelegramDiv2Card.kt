package ru.yandex.alice.kronstadt.scenarios.video_call.cards

import com.yandex.div.dsl.context.CardContext
import com.yandex.div.dsl.context.CardWithTemplates
import com.yandex.div.dsl.context.card
import com.yandex.div.dsl.context.templates
import com.yandex.div.dsl.model.DivAlignmentHorizontal
import com.yandex.div.dsl.model.DivAlignmentVertical
import com.yandex.div.dsl.model.DivContainer
import com.yandex.div.dsl.model.DivCustom
import com.yandex.div.dsl.model.boolIntVariable
import com.yandex.div.dsl.model.divContainer
import com.yandex.div.dsl.model.divCustom
import com.yandex.div.dsl.model.divData
import com.yandex.div.dsl.model.divExtension
import com.yandex.div.dsl.model.divMatchParentSize
import com.yandex.div.dsl.model.state
import com.yandex.div.dsl.type.BoolInt

const val BIND_TO_CALL_VARIABLE = "bindToCall"

object CustomTelegramDiv2Card {
    fun card(
        logId: String,
        customType: String,
        customProps: Map<String, String>,
        extension: String?,
        variable: String? = null,
    ): CardWithTemplates = CardWithTemplates(

        card = card {
            divData(
                logId = logId,
                states = listOf(
                    state(
                        stateId = 0,
                        div = divContainer(
                            width = divMatchParentSize(),
                            height = divMatchParentSize(),
                            orientation = DivContainer.Orientation.OVERLAP,
                            items = listOf(customType(customType, customProps, extension))
                        )
                    )
                ),
                variables = variable?.let {
                    listOf(boolIntVariable(name = variable, BoolInt.TRUE))
                }
            )
        },
        templates = templates { })

    private fun CardContext.customType(
        customType: String,
        customProps: Map<String, String>,
        extension: String?,
    ): DivCustom = divCustom(
        customType = customType,
        customProps = customProps,
        alignmentHorizontal = DivAlignmentHorizontal.LEFT,
        alignmentVertical = DivAlignmentVertical.TOP,
        width = divMatchParentSize(),
        height = divMatchParentSize(),
        extensions = extension?.let {
            listOf(divExtension(id = extension))
        }

    )
}
