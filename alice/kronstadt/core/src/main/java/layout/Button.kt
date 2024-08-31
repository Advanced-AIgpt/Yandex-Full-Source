package ru.yandex.alice.kronstadt.core.layout

import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective
import ru.yandex.alice.kronstadt.core.domain.RewritableAction
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import java.util.Optional

open class Button constructor(
    val text: String,

    // чтобы удобнее конвертировать в винс
    val url: String? = null,
    val payload: String? = null,
    override val semanticFrame: SemanticFrame? = null,
    val hide: Boolean = false,
    override val directives: List<MegaMindDirective> = listOf(),
    override val nluFrameName: String? = null,
    var actionId: String? = null,
    val theme: Theme? = null,
    override val typedSemanticFrame: TypedSemanticFrame?,
    override val parsedUtterance: ParsedUtterance?
) : RewritableAction {

    constructor(
        text: String,
        url: String? = null,
        payload: String? = null,
        semanticFrame: SemanticFrame? = null,
        hide: Boolean = false,
        directives: List<MegaMindDirective> = listOf(),
        nluFrameName: String? = null,
        actionId: String? = null,
        theme: Theme? = null
    ) :
        this(text, url, payload, semanticFrame, hide, directives, nluFrameName, actionId, theme, null, null)

    override fun setActionRef(ref: String) {
        actionId = ref
    }

    override fun toString(): String {
        return "Button(text='$text', url=$url, payload=$payload, semanticFrame=$semanticFrame, hide=$hide, directives=$directives, nluFrameName=$nluFrameName, actionId=$actionId, theme=$theme, typedSemanticFrame=$typedSemanticFrame, parsedUtterance=$parsedUtterance)"
    }

    companion object {
        @JvmStatic
        fun simpleText(text: String, vararg directives: MegaMindDirective): Button {
            return Button(text = text, directives = listOf(*directives))
        }

        @JvmStatic
        fun simpleTextWithNluFrame(
            text: String, nluFrameName: Optional<String>,
            vararg directives: MegaMindDirective
        ): Button {
            return Button(text = text, nluFrameName = nluFrameName.orElse(null), directives = listOf(*directives))
        }

        @JvmStatic
        fun simpleTextWithSemanticFrame(text: String, semanticFrame: SemanticFrame): Button {
            return Button(text = text, semanticFrame = semanticFrame)
        }

        @JvmStatic
        fun simpleTextWithTextDirective(text: String): Button {
            return Button(text = text, directives = listOf(TypeTextDirective(text)))
        }

        @JvmStatic
        fun simpleTextWithTextDirectiveWithImage(text: String, imageUrl: String): Button {
            return Button(text = text, directives = listOf(TypeTextDirective(text)), theme = Theme(imageUrl))
        }

        @JvmStatic
        fun simpleTextWithSemanticFrameWithImage(
            text: String, semanticFrame: SemanticFrame,
            imageUrl: String
        ): Button {
            return Button(text = text, semanticFrame = semanticFrame, theme = Theme(imageUrl))
        }

        @JvmStatic
        fun withUrlAndPayload(
            text: String, url: Optional<String>,
            payload: Optional<String>, hide: Boolean,
            directives: List<MegaMindDirective>
        ): Button {
            return Button(
                text = text,
                url = url.orElse(null),
                payload = payload.orElse(null),
                hide = hide,
                directives = directives
            )
        }

        @JvmStatic
        fun withUrlAndPayload(
            text: String, url: Optional<String>, payload: Optional<String>, hide: Boolean,
            vararg directives: MegaMindDirective
        ): Button {
            return withUrlAndPayload(text, url, payload, hide, listOf(*directives))
        }
    }
}
