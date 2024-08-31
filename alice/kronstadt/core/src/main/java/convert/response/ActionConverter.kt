package ru.yandex.alice.kronstadt.core.convert.response

import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.CallbackDirectiveEffect
import ru.yandex.alice.kronstadt.core.DirectiveListEffect
import ru.yandex.alice.kronstadt.core.NoEffect
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.SemanticFrameEffect
import ru.yandex.alice.kronstadt.core.TypedSemanticFrameEffect
import ru.yandex.alice.megamind.protos.common.FrameProto.TNluPhrase
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TFrameAction
import ru.yandex.alice.protos.data.language.Language

class ActionConverter(
    private val scenarioMeta: ScenarioMeta,
    private val directiveConverter: DirectiveConverter,
    private val callbackDirectiveConverter: CallbackDirectiveConverter,
) : ToProtoConverter<ActionRef, TFrameAction> {

    override fun convert(src: ActionRef, ctx: ToProtoContext): TFrameAction {
        val builder = TFrameAction.newBuilder()

        @Suppress("DEPRECATION")
        when (src.effect) {
            is DirectiveListEffect -> {
                val directivesBuilder = builder.directivesBuilder
                for (dir in src.effect.directives) {
                    directivesBuilder.addList(directiveConverter.convert(dir, ctx))
                }
            }
            is CallbackDirectiveEffect -> {
                builder.setCallback(callbackDirectiveConverter.convert(src.effect.callbackDirective))
            }

            is SemanticFrameEffect -> {
                builder.frame = SemanticFrameToProtoConverter.convert(src.effect.semanticFrame, ctx)
            }
            is TypedSemanticFrameEffect -> {
                builder.parsedUtterance = src.effect.parsedUtterance.toProto(scenarioMeta)
            }
            NoEffect -> {
            }
        }

        builder.nluHintBuilder.frameName = src.nluHint.frameName
        builder.nluHintBuilder.addAllInstances(
            src.nluHint.phrases.map { stringToNluPhrase(it) }
        )
        builder.nluHintBuilder.addAllNegatives(
            src.nluHint.negatives.map { stringToNluPhrase(it) }
        )
        return builder.build()
    }

    private fun stringToNluPhrase(s: String): TNluPhrase {
        return TNluPhrase.newBuilder()
            .setPhrase(s)
            .setLanguage(Language.ELang.L_RUS)
            .build()
    }
}
