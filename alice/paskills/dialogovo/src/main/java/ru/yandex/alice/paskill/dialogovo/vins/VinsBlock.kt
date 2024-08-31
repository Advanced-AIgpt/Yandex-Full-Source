package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonValue
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.paskill.dialogovo.external.v1.response.CardType
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerAction
import ru.yandex.alice.paskill.dialogovo.vins.ClientFeaturesVinsBlock.ClientFeaturesData
import ru.yandex.alice.paskill.dialogovo.vins.ErrorVinsBlock.ErrorBlockData

enum class BlockType(@JsonValue val code: String) {
    SUGGEST("suggest"),
    CLIENT_FEATURES("client_features"),
    DIV_CARD("div_card"),
    AUDIO_PLAYER("audio_player"),
    ERROR("error");
}

@JsonInclude(JsonInclude.Include.NON_ABSENT)
sealed class VinsBlock<T : Any>(
    val type: BlockType,
    val data: T?,
)

class DivCardVinsSlot(data: VinsCard) : VinsBlock<VinsCard>(BlockType.DIV_CARD, data) {
    @JsonProperty("card_template")
    val cardTemplate: CardType = data.type
}

class AudioPlayerVinsBlock(data: AudioPlayerAction?) : VinsBlock<AudioPlayerAction>(BlockType.AUDIO_PLAYER, data)

class ErrorVinsBlock(data: ErrorBlockData) : VinsBlock<ErrorBlockData>(BlockType.ERROR, data) {
    val error = ErrorBlockError.INSTANCE

    class ErrorBlockData(val problems: List<ExternalDevError>)
    class ErrorBlockError(val msg: String, val type: String) {

        companion object {
            val INSTANCE = ErrorBlockError("error", "dialogovo_error")
        }
    }
}

object ClientFeaturesVinsBlock : VinsBlock<ClientFeaturesData>(
    BlockType.CLIENT_FEATURES,
    ClientFeaturesData(
        ClientFeatures(
            divCards = ClientFeature(true),
            intentUrls = ClientFeature(true),
            phoneCalls = ClientFeature(true)
        )
    )
) {

    class ClientFeaturesData(val features: ClientFeatures)

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    data class ClientFeatures(
        @JsonProperty("div_cards")
        val divCards: ClientFeature,
        @JsonProperty("intent_urls")
        val intentUrls: ClientFeature,
        @JsonProperty("phone_calls")
        val phoneCalls: ClientFeature,
    )

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    data class ClientFeature(val enabled: Boolean)
}

enum class SuggestType(@JsonValue private val code: String) {
    EXTERNAL_SKILL_DEACTIVATE("external_skill_deactivate"),
    EXTERNAL_SKILL("external_skill"),
    START_ACCOUNT_LINKING_BUTTON("skill_account_linking_button");
}

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class VinsButton(val title: String, val url: String?, val payload: String?, val hide: Boolean)

// SuggestBlockTypes
abstract class SuggestBlockType<T : Any>(data: T?, @field:JsonProperty("suggest_type") val suggestType: SuggestType) :
    VinsBlock<T>(BlockType.SUGGEST, data)

class StartAccountLinkingBlock(button: Button) : SuggestBlockType<VinsButton>(
    VinsButton(button.text, button.url, button.payload, false), SuggestType.START_ACCOUNT_LINKING_BUTTON
)

class ButtonBlock(button: Button) : SuggestBlockType<VinsButton>(
    VinsButton(button.text, button.url, button.payload, button.hide), SuggestType.EXTERNAL_SKILL
)

object ExternalSkillDeactivateVinsBlock : SuggestBlockType<Any>(null, SuggestType.EXTERNAL_SKILL_DEACTIVATE)
