package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective
import ru.yandex.alice.kronstadt.core.directive.TypeTextSilentDirective
import ru.yandex.alice.kronstadt.core.layout.div.DivAction
import ru.yandex.alice.kronstadt.core.layout.div.DivBody
import ru.yandex.alice.kronstadt.core.layout.div.DivState
import ru.yandex.alice.kronstadt.core.layout.div.block.Block
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.ButtonPressDirective
import java.net.URLEncoder

object DivCardWalker {

    fun walk(visitor: DivBlockVisitor, divBody: DivBody) {
        for (state in divBody.states) {
            walk(visitor, state)
        }
    }

    private fun walk(visitor: DivBlockVisitor, divState: DivState) {
        divState.action?.let { visitor.visit(it) }
        visitor.visit(divState)

        for (block in divState.blocks) {
            walk(visitor, block)
        }
    }

    private fun walk(visitor: DivBlockVisitor, divBlock: Block) {
        divBlock.action?.let { visitor.visit(it) }
        visitor.visit(divBlock)

        for (subBlock in divBlock.subBlocks) {
            walk(visitor, subBlock)
        }
    }
}

interface DivBlockVisitor {
    fun visit(divState: DivState) {}
    fun visit(divAction: DivAction) {}
    fun visit(divBlock: Block) {}
}

private fun toDialogAction(objectMapper: ObjectMapper, vararg directives: Any): String {
    return "dialog-action://?directives=${
    URLEncoder.encode(
        objectMapper.writeValueAsString(directives),
        "UTF-8"
    )
    }"
}

@Component
class OverrideCardButtonUrlDivBlockVisitor(val objectMapper: ObjectMapper) : DivBlockVisitor {
    override fun visit(divAction: DivAction) {
        if (divAction.url.isNullOrBlank()) {
            var text: String? = null
            var payload: String? = null
            for (directive in divAction.directives) {
                when (directive) {
                    is TypeTextSilentDirective -> {
                        text = directive.text
                    }
                    is TypeTextDirective -> {
                        text = directive.text
                    }
                    is ButtonPressDirective -> {
                        text = directive.text
                        payload = directive.payload
                    }
                }
            }

            if (text != null || payload != null) {
                divAction.url =
                    toDialogAction(objectMapper, ConsoleDirective.CardButtonPressDirective(text, payload))
            }
        }
    }
}
