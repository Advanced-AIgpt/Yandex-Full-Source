package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.semanticframes.ButtonAnalytics
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame

sealed class Effect {}

data class DirectiveListEffect(val directives: List<MegaMindDirective>) : Effect()

data class CallbackDirectiveEffect(val callbackDirective: CallbackDirective) : Effect()

@Deprecated("Use TypedSemanticFrameEffect instead")
data class SemanticFrameEffect(val semanticFrame: SemanticFrame) : Effect()

data class TypedSemanticFrameEffect(val parsedUtterance: ParsedUtterance) : Effect()

object NoEffect : Effect()

data class ActionRef internal constructor(
    val effect: Effect,
    val nluHint: NluHint
) {

    data class NluHint @JvmOverloads constructor(
        val frameName: String = "",
        val phrases: List<String> = emptyList(),
        val negatives: List<String> = emptyList()
    )

    object NluHints {
        @JvmField
        val STOP = NluHint(
            frameName = "stop",
            phrases = listOf(
                "хватит",
                "стоп",
                "домой",
                "отбой"
            )
        )

        @JvmField
        val NEXT_STEP = NluHint(
            frameName = "next_step",
            phrases = listOf(
                "давай",
                "дальше"
            )
        )
    }

    companion object {

        @Deprecated("Use TypedSemanticFrame instead")
        @JvmStatic
        fun withSemanticFrame(frame: SemanticFrame, nluHint: NluHint): ActionRef {
            @Suppress("DEPRECATION")
            return ActionRef(SemanticFrameEffect(frame), nluHint)
        }

        @JvmOverloads
        @JvmStatic
        fun withTypedSemanticFrame(
            utterance: String,
            frame: TypedSemanticFrame,
            nluHint: NluHint,
            purpose: String = frame.defaultPurpose()
        ): ActionRef {
            val analytics = ButtonAnalytics(purpose)
            return ActionRef(
                effect = TypedSemanticFrameEffect(ParsedUtterance(utterance, frame, analytics)),
                nluHint = nluHint
            )
        }

        @JvmStatic
        fun withParsedUtterance(
            parsedUtterance: ParsedUtterance,
            nluHint: NluHint,
        ): ActionRef {
            return ActionRef(
                effect = TypedSemanticFrameEffect(parsedUtterance),
                nluHint = nluHint
            )
        }

        @JvmStatic
        fun withDirectives(directives: List<MegaMindDirective>, nluHint: NluHint): ActionRef {
            return ActionRef(
                effect = DirectiveListEffect(directives),
                nluHint = nluHint,
            )
        }

        @JvmStatic
        fun withCallback(callbackDirective: CallbackDirective, nluHint: NluHint): ActionRef {
            return ActionRef(
                effect = CallbackDirectiveEffect(callbackDirective),
                nluHint = nluHint,
            )
        }

        @JvmStatic
        fun noEffect(nluHint: NluHint): ActionRef {
            return ActionRef(effect = NoEffect, nluHint = nluHint)
        }
    }
}
