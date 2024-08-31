package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonRawValue
import com.fasterxml.jackson.databind.annotation.JsonDeserialize
import ru.yandex.alice.kronstadt.core.layout.div.DivBody
import ru.yandex.alice.paskill.dialogovo.utils.AnythingToStringJacksonDeserializer
import java.net.URI

data class VinsResponse(
    val blocks: List<VinsBlock<*>>?,
    val form: VinsForm,
    @JsonInclude(JsonInclude.Include.NON_NULL) val layout: Layout? = null,
    @JsonInclude(JsonInclude.Include.NON_NULL) val endSession: Boolean? = null,
)

@JsonInclude(JsonInclude.Include.NON_EMPTY)
data class Layout(
    @JsonProperty("cards") val cards: List<Card>,
    @JsonProperty("output_speech") val outputSpeech: String?,
    @JsonProperty("should_listen") val shouldListen: Boolean = true,
    @JsonProperty("suggests") val suggests: List<Button> = listOf(),
)

sealed class Card(val type: String) {
    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    data class TextWithButtons(val text: String?, val buttons: List<Button> = listOf()) : Card("text_with_button")

    data class DivCard(val body: DivBody) : Card("div_card")
    // in fact it's
    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto?rev=r7872227#L183
    // data class Div2Card(): Card("div2_card")
}

@JsonInclude(JsonInclude.Include.NON_EMPTY)
data class Button(
    val title: String,
    val url: URI? = null,
    val payload: Any? = null,
    val directives: List<ConsoleDirective> = listOf()
)

sealed class ConsoleDirective(val type: String) {
    data class StartAccountLinking(val url: URI) : ConsoleDirective("start_account_linking")

    data class StartPurchase(@JsonProperty("offer_uuid") val offerUuid: String, val url: URI) :
        ConsoleDirective("start_purchase")

    data class RequestGeosharing(@JsonProperty("period_min") val periodMin: Long) :
        ConsoleDirective("request_geosharing")

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    data class CardButtonPressDirective(
        val text: String? = null,
        @JsonRawValue
        @JsonDeserialize(using = AnythingToStringJacksonDeserializer::class)
        val payload: Any? = null
    ) :
        ConsoleDirective("card_button_press_directive")
}
