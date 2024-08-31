package ru.yandex.alice.paskill.dialogovo.megamind.processor

import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MediaPlayerType
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.kronstadt.core.scenario.NoargScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import java.util.Optional
import java.util.function.Predicate
import kotlin.reflect.KClass

interface RunRequestProcessor<State> : NoargScene<State> {

    override val name: String
        get() = type.name

    override val argsClass: KClass<Any>
        get() = Any::class

    override fun render(request: MegaMindRequest<State>): BaseRunResponse<State>? {
        val ctx = Context(SourceType.USER)
        return process(ctx, request)
    }

    fun hasFrame(frame: String): Predicate<MegaMindRequest<State>> =
        Predicate { request -> request.getSemanticFrame(frame) != null }

    fun hasValueSlot(frame: SemanticFrame, slotType: String): Predicate<MegaMindRequest<State>> =
        Predicate { frame.hasValuedSlot(slotType) }

    fun hasValueSlot(frameName: String, slotType: String): Predicate<MegaMindRequest<State>> =
        Predicate { request -> request.getSemanticFrame(frameName)?.hasValuedSlot(slotType) ?: false }

    fun hasAnyValueSlot(frame: SemanticFrame): Predicate<MegaMindRequest<State>> =
        Predicate { frame.slots.isNotEmpty() }

    fun hasAnyOfFrames(vararg frames: String): Predicate<MegaMindRequest<State>> {
        return Predicate { request: MegaMindRequest<State> -> request.hasAnySemanticFrame(*frames) }
    }

    fun hasSlotEntityType(
        frame: String,
        semanticSlotType: String,
        semanticSlotEntityType: String
    ): Predicate<MegaMindRequest<State>> = Predicate { request ->
        request.getSemanticFrame(frame)
            ?.getTypedEntityValue(semanticSlotType, semanticSlotEntityType) != null
    }

    fun hasOneOfValuesInSlotEntityTypeInAnyFrameOrEmpty(
        semanticSlotType: String,
        semanticSlotEntityType: String,
        acceptedValues: Set<String>
    ): Predicate<MegaMindRequest<State>> = Predicate { request: MegaMindRequest<State> ->
        for (frame in request.input.semanticFrames) {
            val typedEntityValue = frame.getTypedEntityValue(semanticSlotType, semanticSlotEntityType)
            if (typedEntityValue != null) {
                if (acceptedValues.contains(typedEntityValue)) {
                    return@Predicate true
                }
            } else {
                return@Predicate true
            }
        }
        return@Predicate false
    }

    fun hasOneOfValuesInSlotEntityTypeInAnyFrame(
        semanticSlotType: String,
        semanticSlotEntityType: String,
        acceptedValues: Set<String>
    ): Predicate<MegaMindRequest<State>> {
        return Predicate { request: MegaMindRequest<State> ->
            for (frame in request.input.semanticFrames) {
                val typedEntityValue = frame.getTypedEntityValueO(semanticSlotType, semanticSlotEntityType)
                if (typedEntityValue.isPresent) {
                    if (acceptedValues.contains(typedEntityValue.get())) {
                        return@Predicate true
                    }
                }
            }
            return@Predicate false
        }
    }

    fun inAudioPlayerStates(vararg states: AudioPlayerActivityState): Predicate<MegaMindRequest<DialogovoState>> =
        Predicate { request: MegaMindRequest<DialogovoState> -> request.inAudioPlayerStates(*states) }

    fun hasExperiment(exp: String): Predicate<MegaMindRequest<DialogovoState>> =
        Predicate { request: MegaMindRequest<DialogovoState> -> request.hasExperiment(exp) }

    fun onInputClass(clazz: Class<out Input>): Predicate<MegaMindRequest<DialogovoState>> =
        Predicate { request: MegaMindRequest<DialogovoState> -> request.input.javaClass.isAssignableFrom(clazz) }

    fun onCallbackDirective(clazz: Class<out CallbackDirective>): Predicate<MegaMindRequest<DialogovoState>> =
        Predicate { request: MegaMindRequest<DialogovoState> -> request.input.isCallback(clazz) }

    fun canProcess(request: MegaMindRequest<State>): Boolean

    val type: ProcessorType

    fun process(context: Context, request: MegaMindRequest<State>): BaseRunResponse<State>

    companion object {
        @JvmStatic
        fun getSkillId(request: MegaMindRequest<DialogovoState>): Optional<String> {
            return request.getDialogIdO()
                .or { request.getStateO().flatMap { obj: DialogovoState -> obj.getCurrentSkillIdO() } }
        }

        // todo: replace "isCentaur" with new supported feature
        @JvmField
        val IS_IN_SKILL = Predicate { request: MegaMindRequest<DialogovoState> ->
            if (request.clientInfo.isCentaur) {
                request.hasSemanticFrame(SemanticFrames.EXTERNAL_SKILL_SESSION_REQUEST)
            } else {
                request.dialogId != null ||
                    (request.state?.currentSkillId != null && !(request.state?.sessionInBackground ?: false))
            }
        }

        @JvmField
        public final val HAS_CURRENT_SKILL_SESSION = Predicate { request: MegaMindRequest<DialogovoState> ->
            request.state?.currentSkillId != null
        }

        @JvmField
        val IS_PP = Predicate { request: MegaMindRequest<DialogovoState> -> request.clientInfo.isSearchApp }

        @JvmField
        val IS_NAVI_OR_MAPS =
            Predicate { request: MegaMindRequest<DialogovoState> -> request.clientInfo.isNavigatorOrMaps }

        @JvmField
        val IS_SMART_SPEAKER_OR_TV =
            Predicate { request: MegaMindRequest<DialogovoState> -> request.clientInfo.isYaSmartDevice }

        @JvmField
        val IS_CENTAUR =
            Predicate { request: MegaMindRequest<DialogovoState> -> request.clientInfo.isCentaur }

        @JvmField
        val SURFACE_SUPPORTS_DIV_CARDS =
            Predicate { request: MegaMindRequest<DialogovoState> -> request.clientInfo.supportsDivCards() }

        @JvmField
        val HAS_SKILL_PLAYER_OWNER =
            Predicate { request: MegaMindRequest<DialogovoState> -> request.getAudioPlayerOwnerSkillIdO().isPresent }

        // literally - is last user play command was on new thin audio player rather than music|video|radio or not
        // checked by last_play_timestamp field on
        // device_state.(audio_player|video_player|radio_player|music_player).currently_playing.last_play_timestamp
        // if none is last_active - new audio_player wins
        @JvmField
        val AUDIO_PLAYER_IS_LAST_MEDIA_PLAYER_ACTIVE = Predicate { request: MegaMindRequest<DialogovoState> ->
            request.getLastActiveMediaPlayer()?.let { it == MediaPlayerType.AUDIO_PLAYER } ?: true
        }

        // use with HAS_SKILL_PLAYER_OWNER=true and IS_IN_SKILL=true
        @JvmField
        val IS_IN_PLAYER_OWNER_SKILL = Predicate { request: MegaMindRequest<DialogovoState> ->
            val currentlyInSkillIdO = getSkillId(request)
            val audioPlayerOwnerSkillIdO = request.getAudioPlayerOwnerSkillIdO()

            // additional checks
            if (currentlyInSkillIdO.isEmpty) {
                return@Predicate true
            }
            if (audioPlayerOwnerSkillIdO.isEmpty) {
                return@Predicate false
            }
            val currentlyInSkillId = currentlyInSkillIdO.get()
            val audioPlayerOwnerSkillId = audioPlayerOwnerSkillIdO.get()
            return@Predicate currentlyInSkillId == audioPlayerOwnerSkillId
        }

        @JvmField
        val COUNTRY_RUSSIA_IF_SPECIFIED = Predicate { request: MegaMindRequest<DialogovoState> ->
            val userRegionO = request.getUserRegionO()
            if (userRegionO.isEmpty) {
                return@Predicate true
            }
            return@Predicate userRegionO.get().countryId == LaasCountry.RUSSIA.id
        }
    }
}
