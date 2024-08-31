package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.databind.annotation.JsonDeserialize
import com.fasterxml.jackson.databind.annotation.JsonSerialize
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.Directive
import ru.yandex.alice.kronstadt.core.utils.ActivationTypedSemanticFrameFromBase64StringDeserializer
import ru.yandex.alice.kronstadt.core.utils.ActivationTypedSemanticFrameToBase64StringSerializer
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import java.util.Optional

@Directive("new_dialog_session")
data class NewSessionDirective(
    @JsonProperty("dialog_id")
    val dialogId: String,
    @JsonProperty("request")
    val request: String? = null,
    @JsonProperty("original_utterance")
    val originalUtterance: String? = null,
    @JsonProperty("source")
    val source: Optional<String> = Optional.empty(),
    @JsonProperty("payload")
    val payload: Optional<String> = Optional.empty(),
    @JsonDeserialize(using = ActivationTypedSemanticFrameFromBase64StringDeserializer::class)
    @JsonSerialize(using = ActivationTypedSemanticFrameToBase64StringSerializer::class)
    @JsonProperty("activation_typed_semantic_frame")
    val activationTypedSemanticFrame: TTypedSemanticFrame? = null
) : CallbackDirective
