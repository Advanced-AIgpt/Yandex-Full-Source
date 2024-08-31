package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonSubTypes
import com.fasterxml.jackson.annotation.JsonTypeInfo
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.node.NullNode
import lombok.Data
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState
import ru.yandex.alice.paskill.dialogovo.vins.MarkupVinsSlot.VinsMarkup
import ru.yandex.alice.paskill.dialogovo.vins.ResponseVinsSlot.ResponseVinsSlotData
import ru.yandex.alice.paskill.dialogovo.vins.SessionVinsSlot.VinsSession
import ru.yandex.alice.paskill.dialogovo.vins.SkillMetaSlot.SkillMeta

const val SESSION = "session"
const val SKILL_DEBUG = "skill_debug"
const val REQUEST = "request"
const val RESPONSE = "response"
const val VINS_MARKUP = "vins_markup"

// public static final String UTTARANCE_NORMALIZED = "utterance_normalized";
const val SKILL_ID = "skill_id"
const val SKILL_META = "skill_meta"
const val RANDOM_SEED = "random_seed"
const val AUDIO_PLAYER = "audio_player"

@Data
@JsonInclude(JsonInclude.Include.NON_ABSENT)
@JsonTypeInfo(
    use = JsonTypeInfo.Id.NAME,
    property = "name",
    include = JsonTypeInfo.As.EXISTING_PROPERTY,
    defaultImpl = UnknownSlot::class
)
@JsonSubTypes(
    JsonSubTypes.Type(value = SessionVinsSlot::class, name = SESSION),
    JsonSubTypes.Type(value = MarkupVinsSlot::class, name = VINS_MARKUP),
    JsonSubTypes.Type(value = RequestVinsSlot::class, name = REQUEST),
    JsonSubTypes.Type(value = SkillIdVinsSlot::class, name = SKILL_ID),
    JsonSubTypes.Type(value = SkillMetaSlot::class, name = SKILL_META),
    JsonSubTypes.Type(value = RandomSeedSlot::class, name = RANDOM_SEED),
    JsonSubTypes.Type(value = AudioPlayerStateSlot::class, name = AUDIO_PLAYER)
)
sealed class VinsSlot<T>(
    val name: String,
    val type: String,
    val optional: Boolean = false,
    open val value: T? = null,
)

class UnknownSlot(value: Any?) :
    VinsSlot<Any>(SESSION, SESSION, false, SKIPPED) {
    companion object {
        private val SKIPPED = Any()
    }
}

class SessionVinsSlot(value: VinsSession?) :
    VinsSlot<VinsSession>(SESSION, SESSION, false, value) {
    data class VinsSession(val id: String, val seq: Long, @JsonProperty("isEnded") val ended: Boolean)
}

class MarkupVinsSlot(value: VinsMarkup?) :
    VinsSlot<VinsMarkup>(VINS_MARKUP, VINS_MARKUP, true, value) {

    data class VinsMarkup(
        @JsonProperty("dangerous_context")
        val isDangerousContext: Boolean = false
    )
}

class RequestVinsSlot(
    type: String,
    override val value: JsonNode = NullNode.instance,
) : VinsSlot<JsonNode>(REQUEST, type, true, value)

class SkillIdVinsSlot(value: String?) :
    VinsSlot<String>(SKILL_ID, "skill", false, value)

class SkillMetaSlot(value: SkillMeta?) :
    VinsSlot<SkillMeta>(SESSION, SESSION, false, value) {

    data class SkillMeta(
        val skillId: String,
        @JsonProperty("isDraft")
        val isDraft: Boolean = false,
    )
}

class AudioPlayerStateSlot(value: AudioPlayerState?) :
    VinsSlot<AudioPlayerState>(AUDIO_PLAYER, "audio_player", true, value)

class DebugVinsSlot(value: DebugInfo?) :
    VinsSlot<DebugVinsSlot.DebugInfo>(SKILL_DEBUG, "json", true, value) {

    data class DebugInfo(
        @JsonProperty("response_raw")
        val responseRaw: String?,
        val request: Any?,
    )
}

class ResponseVinsSlot(value: ResponseVinsSlotData?) :
    VinsSlot<ResponseVinsSlotData>(RESPONSE, RESPONSE, true, value) {

    data class ResponseVinsSlotData(
        val text: String?,
        val voice: String?
    )
}

/**
 * Fake vins slot for unit tests.
 */
class RandomSeedSlot(value: Int?) : VinsSlot<Int>(RANDOM_SEED, "random_seed", true, value)
