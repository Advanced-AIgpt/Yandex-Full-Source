package ru.yandex.alice.kronstadt.core.convert.response

import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionRef.NluHint
import ru.yandex.alice.kronstadt.core.domain.RewritableAction

class ToProtoContext {
    private val actions: MutableMap<String, ActionRef> = hashMapOf()

    internal fun getActions(): Map<String, ActionRef> {
        return actions
    }

    internal fun rewriteAction(action: RewritableAction): String {
        val directives = action.directives
        val ref = nextActionRef()
        val nluFrameName = action.nluFrameName ?: ref
        action.setActionRef(ref)
        val semanticFrame = action.semanticFrame
        if (semanticFrame != null) {
            actions[ref] =
                @Suppress("DEPRECATION") ActionRef.withSemanticFrame(semanticFrame, NluHint(nluFrameName))
        } else {
            val parsedUtterance = action.parsedUtterance
            if (parsedUtterance != null) {
                actions[ref] = ActionRef.withParsedUtterance(parsedUtterance, NluHint(nluFrameName))
            } else {
                actions[ref] = ActionRef.withDirectives(directives, NluHint(nluFrameName))
            }
        }
        return ref
    }

    private fun nextActionRef(): String = "action_" + (actions.size + 1)
}
