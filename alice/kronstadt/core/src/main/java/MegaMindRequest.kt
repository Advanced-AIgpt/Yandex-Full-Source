package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.domain.Age
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState
import ru.yandex.alice.kronstadt.core.domain.BlackboxInfo
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.domain.ContactsList
import ru.yandex.alice.kronstadt.core.domain.FiltrationLevel
import ru.yandex.alice.kronstadt.core.domain.LocationInfo
import ru.yandex.alice.kronstadt.core.domain.Options
import ru.yandex.alice.kronstadt.core.domain.QuasarAuxiliaryConfig
import ru.yandex.alice.kronstadt.core.domain.SkillDiscoveryCandidates
import ru.yandex.alice.kronstadt.core.domain.UserRegion
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.megamind.protos.common.IoT
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioBaseRequest.ERequestSourceType
import java.time.Duration
import java.time.Instant
import java.util.Optional
import java.util.Random
import kotlin.random.asKotlinRandom

data class MegaMindRequest<State>(
    override val requestId: String,
    override val scenarioMeta: ScenarioMeta,

    val dialogId: String? = null,
    override val serverTime: Instant = Instant.now(),
    override val randomSeed: Long = kotlin.random.Random.nextLong(),
    override val experiments: Set<String> = setOf(),
    override val clientInfo: ClientInfo,

    private val locationInfo: LocationInfo? = null,

    val state: State? = null,
    val input: Input,
    val voiceSession: Boolean = false,
    val mordoviaState: Map<String, Any?> = mapOf(),
    val spotter: String = "yandex",
    val options: Options = Options(QuasarAuxiliaryConfig(null)),
    private val deviceStateLazy: Lazy<DeviceState?> = lazyOf(null),

    val filtrationLevel: FiltrationLevel = FiltrationLevel.MODERATE,
    val userId: String? = null,
    val userClassification: UserClassification = UserClassification(Age.ADULT),

    // Shows if previous scenario was the same
    val newScenarioSession: Boolean = false,

    override val random: Random = Random(randomSeed),

    private val userRegionLazy: Lazy<UserRegion?> = lazyOf(null),
    private val blackboxInfoLazy: Lazy<BlackboxInfo?> = lazyOf(null),

    private val skillDiscoveryGcCandidatesLazy: Lazy<SkillDiscoveryCandidates?> = lazyOf(null),
    override val mementoData: RequestProto.TMementoData = RequestProto.TMementoData.getDefaultInstance(),
    val requestSource: RequestSource = RequestSource.DEFAULT,

    val stackOwner: Boolean = false,
    private val iotUserInfoLazy: Lazy<IoT.TIoTUserInfo?> = lazyOf(null),
    private val contactsListLazy: Lazy<ContactsList?> = lazyOf(null),
    private val videoCallCapabilityLazy: Lazy<VideoCallCapability?> = lazyOf(null),
    override val additionalSources: AdditionalSources = AdditionalSources.EMPTY,

    ) : ScenarioRequest {

    val deviceState: DeviceState? by deviceStateLazy
    val userRegion: UserRegion? by userRegionLazy

    val blackboxInfo: BlackboxInfo? by blackboxInfoLazy
    val skillDiscoveryGcCandidates: SkillDiscoveryCandidates? by skillDiscoveryGcCandidatesLazy
    val iotUserInfo: IoT.TIoTUserInfo? by iotUserInfoLazy
    val contactsList: ContactsList? by contactsListLazy
    val videoCallCapability: VideoCallCapability? by videoCallCapabilityLazy

    override val krandom: kotlin.random.Random = random.asKotlinRandom()

    private val audioPlayerOwnerSkillId: String?
        get() = deviceState?.audioPlayerState?.meta?.get(DeviceState.PLAYER_STATE_META_SKILL_ID_FIELD)
    private val audioPlayerActivityState: AudioPlayerActivityState?
        get() = deviceState?.audioPlayerState?.activityState
    private val audioPlayerLastStopTimestamp: Instant?
        get() = deviceState?.audioPlayerState?.lastStopTimestamp
    val audioPlayerLastPlayTimestamp: Instant?
        get() = deviceState?.audioPlayerState?.lastPlayTimestamp

    // taken from https://a.yandex-team.ru/arc/trunk/arcadia/alice/vins/apps/personal_assistant
    // /personal_assistant/nlg_globals.py?rev=6345238#L209-225
    val assistantName: String
        get() = if ("yandex" == spotter) "Яндекс" else "Алиса"

    fun getStateO() = Optional.ofNullable(state)
    fun getDialogIdO() = Optional.ofNullable(dialogId)
    fun getUserRegionO() = Optional.ofNullable(userRegion)
    fun getSkillDiscoveryGcCandidatesO() = Optional.ofNullable(skillDiscoveryGcCandidates)
    fun getDeviceStateO() = Optional.ofNullable(deviceState)
    fun getUserIdO() = Optional.ofNullable(userId)

    fun isTest(): Boolean = requestId.startsWith("ffffffff-ffff") || clientInfo.uuid.startsWith("deadbeaf")

    // biometry score or PP settings
    fun isChildMode() = userClassification.age == Age.CHILD || filtrationLevel == FiltrationLevel.SAFE

    fun getLastActiveMediaPlayer(): MediaPlayerType? = sequenceOf(
        MediaPlayerType.AUDIO_PLAYER to deviceState?.audioPlayerState?.lastPlayTimestamp,
        MediaPlayerType.MUSIC to deviceState?.music?.lastPlayTimestamp,
        MediaPlayerType.VIDEO to deviceState?.video?.lastPlayTimestamp,
        MediaPlayerType.RADIO to deviceState?.radio?.currentlyPlaying?.lastPlayTimestamp
    )
        .filter { it.second != null }
        .maxByOrNull { (_, lastPlay) -> lastPlay?.toEpochMilli() ?: 0 }
        ?.first

    fun getAudioPlayerOwnerSkillIdO(): Optional<String> = Optional.ofNullable(audioPlayerOwnerSkillId)

    fun getLastStopTimestampO(): Optional<Instant> = Optional.ofNullable(audioPlayerLastStopTimestamp)
    fun getLastPlayTimestampO(): Optional<Instant> = Optional.ofNullable(audioPlayerLastPlayTimestamp)
    fun getLocationInfoO() = Optional.ofNullable(locationInfo)
    fun isNewScenarioSession() = newScenarioSession
    fun isStackOwner() = stackOwner

    fun inAudioPlayerStates(vararg states: AudioPlayerActivityState): Boolean {
        val currentState = audioPlayerActivityState
        return states.any { expected -> expected == currentState }
    }

    fun isSkillPlayerOwner(skillId: String): Boolean {
        val audioPlayerOwnerSkillIdO = getAudioPlayerOwnerSkillIdO()
        return audioPlayerOwnerSkillIdO.isPresent && audioPlayerOwnerSkillIdO.get() == skillId
    }

    fun isVoiceSession() = voiceSession

    fun hasActiveAudioPlayer(): Boolean = clientInfo.isYaSmartDevice
        && getAudioPlayerOwnerSkillIdO().isPresent
        && inAudioPlayerStates(AudioPlayerActivityState.PLAYING)

    fun getSemanticFrame(semanticFrameType: String): SemanticFrame? =
        input.semanticFrames.firstOrNull { sf -> semanticFrameType == sf.name }

    fun getSemanticFrameO(semanticFrameType: String): Optional<SemanticFrame> =
        Optional.ofNullable(getSemanticFrame(semanticFrameType))

    fun hasSemanticFrame(semanticFrameType: String): Boolean =
        input.semanticFrames.any { sf -> sf.name == semanticFrameType }

    fun hasAnySemanticFrame(vararg semanticFrameTypes: String): Boolean {
        val expectedSFNames = semanticFrameTypes.toSet()
        return input.semanticFrames.any { expectedSFNames.contains(it.name) }
    }

    fun hasAnySemanticFrame(semanticFrameTypes: Collection<String>): Boolean {
        val expectedSFNames = semanticFrameTypes.toSet()
        return input.semanticFrames.any { expectedSFNames.contains(it.name) }
    }

    fun getAnySemanticFrame(semanticFrameTypes: Collection<String>): SemanticFrame? {
        val expectedSFNames = semanticFrameTypes.toSet()
        return input.semanticFrames.firstOrNull { expectedSFNames.contains(it.name) }
    }

    fun getAnySemanticFrameO(semanticFrameTypes: Collection<String>): Optional<SemanticFrame> {
        return Optional.ofNullable(getAnySemanticFrame(semanticFrameTypes))
    }

    fun getAnySemanticFrameO(vararg semanticFrameTypes: String): Optional<SemanticFrame> {
        val expectedSFNames = semanticFrameTypes.toSet()
        return Optional.ofNullable(input.semanticFrames.firstOrNull { expectedSFNames.contains(it.name) })
    }

    // thick music player
    fun isMusicPlayerCurrentlyPlaying(): Boolean = !(deviceState?.music?.player?.pause ?: true)

    // thin music player
    fun isAudioPlayerCurrentlyPlaying(): Boolean = audioPlayerActivityState == AudioPlayerActivityState.PLAYING

    fun isRadioPlayerCurrentlyPlaying(): Boolean = !(deviceState?.radio?.player?.pause ?: true)

    fun isAnyPlayerCurrentlyPlaying(): Boolean = isMusicPlayerCurrentlyPlaying() ||
        isAudioPlayerCurrentlyPlaying() ||
        isRadioPlayerCurrentlyPlaying()

    class DeviceState constructor(
        val soundLevel: Int,
        val soundMuted: Boolean,
        music: Lazy<Music?>,
        video: Lazy<Video?>,
        radio: Lazy<Radio?>,
        val audioPlayerState: AudioPlayer? = null,
        val isTvPluggedIn: Boolean = false,
        val activeTimers: List<Timer> = listOf(),
    ) {

        constructor(
            soundLevel: Int,
            soundMuted: Boolean,
            music: Music? = null,
            video: Video? = null,
            radio: Radio? = null,
            audioPlayerState: AudioPlayer? = null,
            isTvPluggedIn: Boolean = false,
            activeTimers: List<Timer> = listOf(),
        ) : this(
            soundLevel = soundLevel,
            soundMuted = soundMuted,
            music = lazyOf(music),
            video = lazyOf(video),
            radio = lazyOf(radio),
            audioPlayerState = audioPlayerState,
            isTvPluggedIn = isTvPluggedIn,
            activeTimers = activeTimers,
        )

        val music: Music? by music
        val video: Video? by video
        val radio: Radio? by radio

        val isCurrentlyPlayingAnything: Boolean
            get() = music?.currentlyPlaying != null ||
                video?.currentlyPlaying != null ||
                audioPlayerState?.currentlyPlaying != null

        data class AudioPlayer(
            val token: String? = null,
            val offsetMs: Long,
            val activityState: AudioPlayerActivityState? = null,
            val meta: Map<String, String> = mapOf(),
            val currentlyPlaying: CurrentlyPlaying? = null,
            val lastPlayTimestamp: Instant? = null,
            val lastStopTimestamp: Instant? = null,
        )

        data class Timer(
            val timerId: String? = null,
            val startTimestamp: Instant? = null,
            val duration: Duration? = null,
            val remaining: Duration? = null,
            val currentlyPlaying: Boolean = false,
            val paused: Boolean = false,
        ) {
            fun isCurrentlyPlaying() = currentlyPlaying
        }

        companion object {
            const val PLAYER_STATE_META_SKILL_ID_FIELD = "skillId"
            const val TRACK_OFFSET_MS_ON_START = "trackOffsetMsOnStart"
        }
    }

    data class UserClassification(val age: Age? = null)

    data class Music(
        val player: Player? = null,
        val currentlyPlaying: CurrentlyPlaying? = null,
        val lastPlayTimestamp: Instant? = null,
        val lastStopTimestamp: Instant? = null,
    ) {
        fun getPlayerO() = Optional.ofNullable(player)
        fun getCurrentlyPlayingO() = Optional.ofNullable(currentlyPlaying)
        fun getLastPlayTimestampO() = Optional.ofNullable(lastPlayTimestamp)
        fun getLastStopTimestampO() = Optional.ofNullable(lastStopTimestamp)
    }

    data class Video(
        val currentlyPlaying: CurrentlyPlaying? = null,
        val lastPlayTimestamp: Instant? = null,
        val lastStopTimestamp: Instant? = null,
    ) {
        fun getCurrentlyPlayingO() = Optional.ofNullable(currentlyPlaying)
        fun getLastPlayTimestampO() = Optional.ofNullable(lastPlayTimestamp)
        fun getLastStopTimestampO() = Optional.ofNullable(lastStopTimestamp)
    }

    data class Radio(
        val player: Player? = null,
        val currentlyPlaying: CurrentlyPlaying?
    ) {
        fun getPlayerO() = Optional.ofNullable(player)
        fun getCurrentlyPlayingO() = Optional.ofNullable(currentlyPlaying)
    }

    data class CurrentlyPlaying(val lastPlayTimestamp: Instant?)

    data class Player(val pause: Boolean)

    /**
     * The flag is set in requests from stack-engine (parovoz) callbacks
     * @see ERequestSourceType
     */
    enum class RequestSource {
        DEFAULT, GET_NEXT;

        companion object {
            internal fun fromProtoSource(source: ERequestSourceType) =
                when (source) {
                    ERequestSourceType.GetNext -> GET_NEXT
                    else -> DEFAULT
                }
        }
    }
}
