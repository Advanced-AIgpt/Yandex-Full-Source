package ru.yandex.alice.kronstadt.core.domain

import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame

interface RewritableAction {
    val directives: List<MegaMindDirective>
    val nluFrameName: String?
    val semanticFrame: SemanticFrame?
    val typedSemanticFrame: TypedSemanticFrame?
    val parsedUtterance: ParsedUtterance?
    fun setActionRef(ref: String)
}
