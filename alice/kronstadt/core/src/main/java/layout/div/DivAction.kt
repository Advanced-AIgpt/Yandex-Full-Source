package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.domain.RewritableAction
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class DivAction(
    @JsonProperty("log_id")
    val logId: String,/*?*/

    // replaced by mm_deeplink
    var url: String? = null,

    @JsonIgnore
    override val directives: List<MegaMindDirective> = listOf(),

    @JsonIgnore
    override val typedSemanticFrame: TypedSemanticFrame? = null,

    @JsonIgnore
    override val parsedUtterance: ParsedUtterance? = null,
) : RewritableAction {

    constructor(logId: String, url: String?, vararg directives: MegaMindDirective) : this(
        logId,
        url,
        directives = directives.asList(),
        typedSemanticFrame = null,
        parsedUtterance = null
    )

    constructor(logId: String, url: String?, directives: List<MegaMindDirective>) : this(
        logId = logId,
        url = url,
        directives = directives,
        typedSemanticFrame = null,
        parsedUtterance = null,
    )

    constructor(logId: String, url: String?, parsedUtterance: ParsedUtterance) : this(
        logId = logId,
        url = url,
        directives = listOf(),
        typedSemanticFrame = null,
        parsedUtterance = parsedUtterance,
    )

    override fun setActionRef(ref: String) {
        url = "@@mm_deeplink#$ref"
    }

    override val nluFrameName: String?
        get() = null
    override val semanticFrame: SemanticFrame?
        get() = null
}
