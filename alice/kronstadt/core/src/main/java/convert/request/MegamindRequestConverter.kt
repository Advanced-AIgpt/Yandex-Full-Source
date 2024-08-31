package ru.yandex.alice.kronstadt.core.convert.request

import com.google.protobuf.Struct
import org.apache.logging.log4j.LogManager
import org.apache.logging.log4j.Logger
import org.apache.logging.log4j.util.Strings
import ru.yandex.alice.kronstadt.core.AdditionalSources
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.MegaMindRequest.DeviceState
import ru.yandex.alice.kronstadt.core.MegaMindRequest.Player
import ru.yandex.alice.kronstadt.core.MegaMindRequest.Radio
import ru.yandex.alice.kronstadt.core.MegaMindRequest.RequestSource
import ru.yandex.alice.kronstadt.core.MegaMindRequest.UserClassification
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.convert.StateConverter
import ru.yandex.alice.kronstadt.core.domain.Age
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState
import ru.yandex.alice.kronstadt.core.domain.BlackboxInfo
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.domain.ContactsList
import ru.yandex.alice.kronstadt.core.domain.FiltrationLevel
import ru.yandex.alice.kronstadt.core.domain.Interfaces
import ru.yandex.alice.kronstadt.core.domain.LocationInfo
import ru.yandex.alice.kronstadt.core.domain.Options
import ru.yandex.alice.kronstadt.core.domain.QuasarAuxiliaryConfig
import ru.yandex.alice.kronstadt.core.domain.QuasarAuxiliaryConfig.Alice4BusinessConfig
import ru.yandex.alice.kronstadt.core.domain.SkillDiscoveryCandidate
import ru.yandex.alice.kronstadt.core.domain.SkillDiscoveryCandidates
import ru.yandex.alice.kronstadt.core.domain.UserRegion
import ru.yandex.alice.library.client.protos.ClientInfoProto
import ru.yandex.alice.megamind.protos.common.DataSourceType
import ru.yandex.alice.megamind.protos.common.DeviceStateProto.TDeviceState
import ru.yandex.alice.megamind.protos.common.DeviceStateProto.TDeviceState.TAudioPlayer
import ru.yandex.alice.megamind.protos.common.DeviceStateProto.TDeviceState.TAudioPlayer.TPlayerState
import ru.yandex.alice.megamind.protos.common.DeviceStateProto.TDeviceState.TTimers
import ru.yandex.alice.megamind.protos.common.EnvironmentStateProto
import ru.yandex.alice.megamind.protos.common.IoT
import ru.yandex.alice.megamind.protos.quasar.AuxiliaryConfig.TAuxiliaryConfig
import ru.yandex.alice.megamind.protos.quasar.AuxiliaryConfig.TAuxiliaryConfig.TAlice4BusinessConfig
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TDataSource
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioBaseRequest
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TUserClassification.EAge
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TUserPreferences.EFiltrationMode
import ru.yandex.alice.protos.endpoint.CapabilityProto
import java.time.Duration
import java.time.Instant

open class MegamindRequestConverter<State>(
    private val scenarioMeta: ScenarioMeta,
    private val inputConverter: InputConverter,
    private val protoUtil: ProtoUtil,
    private val stateConverter: StateConverter<State>,
    private val contactsListConverter: ContactsListConverter,
    private val videoCallCapabilityConverter: VideoCallCapabilityConverter,
) {

    protected val logger: Logger = LogManager.getLogger(MegamindRequestConverter::class.java)

    fun convertRunRequest(
        src: RequestProto.TScenarioRunRequest,
        userId: String?,
        additionalSources: AdditionalSources
    ): MegaMindRequest<State> {
        return getMegaMindRequest(userId, src.baseRequest, src.input, src.dataSourcesMap, additionalSources)
    }

    fun getMegaMindRequest(
        userId: String?,
        baseRequest: TScenarioBaseRequest,
        input: RequestProto.TInput,
        dataSourcesMap: Map<Int, TDataSource>,
        additionalSources: AdditionalSources,
    ): MegaMindRequest<State> {
        val viewState = baseRequest.deviceState.video.viewState

        // MM doesn't send Scenario so we can't filter foreign states :(
        val mordoviaState =  //"Dialogovo".equals(baseRequest.getDeviceState().getVideo().getScenario
            // ()) &&
            if (Struct.getDefaultInstance() != viewState) protoUtil.structToMap(viewState) else emptyMap()
        val filtrationLevel = getFiltrationLevel(baseRequest)
        val userClassification = getUserClassification(baseRequest)
        return MegaMindRequest(
            requestId = baseRequest.requestId,
            scenarioMeta = scenarioMeta,
            dialogId = baseRequest.dialogId.ifEmpty { null },
            serverTime = Instant.ofEpochMilli(baseRequest.serverTimeMs),
            randomSeed = baseRequest.randomSeed,
            locationInfo = getLocation(baseRequest),
            experiments = baseRequest.experiments.fieldsMap.keys,
            clientInfo = getClientInfo(baseRequest), //I wonder why we check interfaces but not input type...
            voiceSession = baseRequest.interfaces.voiceSession,
            mordoviaState = mordoviaState,
            state = stateConverter.convert(baseRequest),
            input = inputConverter.convert(input),
            spotter = baseRequest.deviceState.deviceConfig.spotter,
            options = getOptions(baseRequest),
            deviceStateLazy = lazy { getDeviceState(baseRequest.deviceState, baseRequest) },
            filtrationLevel = filtrationLevel,
            userId = userId,
            userClassification = userClassification,
            newScenarioSession = baseRequest.isNewSession,
            userRegionLazy = lazy { getUserRegion(dataSourcesMap) },
            blackboxInfoLazy = lazy { getBlackboxInfo(dataSourcesMap) },
            skillDiscoveryGcCandidatesLazy = lazy { getDiscoveryGcCandidates(dataSourcesMap) },
            mementoData = baseRequest.memento,
            requestSource = RequestSource.fromProtoSource(baseRequest.requestSource),
            stackOwner = baseRequest.isStackOwner,
            iotUserInfoLazy = lazy { getIotUserInfo(dataSourcesMap) },
            contactsListLazy = lazy { getContactsList(dataSourcesMap) },
            videoCallCapabilityLazy = lazy { getVideoCallCapability(dataSourcesMap, baseRequest.clientInfo) },
            additionalSources = additionalSources,
        )
    }

    private fun getUserClassification(baseRequest: TScenarioBaseRequest): UserClassification {
        return UserClassification(mapAge(baseRequest.userClassification.age))
    }

    @Suppress("DEPRECATION")
    private fun getFiltrationLevel(baseRequest: TScenarioBaseRequest): FiltrationLevel {
        return if (baseRequest.userPreferences.filtrationMode == EFiltrationMode.UNRECOGNIZED) {
            // something not default is passed but we are not expecting it, so fallback to default
            FiltrationLevel.MODERATE
        } else if (baseRequest.userPreferences.filtrationMode == EFiltrationMode.NoFilter &&
            baseRequest.options.filtrationLevel > 0
        ) {
            when (baseRequest.options.filtrationLevel) {
                0 -> FiltrationLevel.NO_FILTER
                1 -> FiltrationLevel.MODERATE
                2 -> FiltrationLevel.FAMILY_SEARCH
                else -> FiltrationLevel.MODERATE
            }
        } else {
            when (baseRequest.userPreferences.filtrationMode) {
                EFiltrationMode.NoFilter -> FiltrationLevel.NO_FILTER
                EFiltrationMode.Moderate -> FiltrationLevel.MODERATE
                EFiltrationMode.FamilySearch -> FiltrationLevel.FAMILY_SEARCH
                EFiltrationMode.Safe -> FiltrationLevel.SAFE
                else -> FiltrationLevel.MODERATE
            }
        }
    }

    private fun getOptions(baseRequest: TScenarioBaseRequest) =
        Options(getQuasarAuxiliaryConfig(baseRequest.options.quasarAuxiliaryConfig))

    private fun getQuasarAuxiliaryConfig(auxiliaryConfig: TAuxiliaryConfig) =
        QuasarAuxiliaryConfig(getAlice4Business(auxiliaryConfig.alice4Business))

    private fun getAlice4Business(alice4BusinessConfig: TAlice4BusinessConfig): Alice4BusinessConfig? {
        return if (alice4BusinessConfig != alice4BusinessConfig.defaultInstanceForType)
            Alice4BusinessConfig(
                alice4BusinessConfig.preactivatedSkillIdsList,
                alice4BusinessConfig.unlocked
            )
        else null
    }

    private fun getDeviceState(
        deviceState: TDeviceState,
        request: TScenarioBaseRequest
    ): DeviceState? {
        if (deviceState == deviceState.defaultInstanceForType) {
            return null
        }
        val timers = deviceState.timers.activeTimersList
            .map { protoTimer: TTimers.TTimer ->
                DeviceState.Timer(
                    protoTimer.timerId,
                    Instant.ofEpochSecond(protoTimer.startTimestamp),
                    Duration.ofSeconds(protoTimer.duration.toLong()),
                    Duration.ofSeconds(protoTimer.remaining.toLong()),
                    protoTimer.currentlyPlaying,
                    protoTimer.paused
                )
            }
        return DeviceState(
            deviceState.soundLevel,
            deviceState.soundMuted,
            getMusic(deviceState),
            getVideo(deviceState),
            getRadio(deviceState),
            getAudioPlayer(deviceState, request),
            deviceState.hasIsTvPluggedIn() && deviceState.isTvPluggedIn,
            timers
        )
    }

    private fun getVideoCallCapability(
        dataSourcesMap: Map<Int, TDataSource>,
        clientInfo: ClientInfoProto.TClientInfoProto
    ): VideoCallCapability? {
        val environmentState = getEnvironmentState(dataSourcesMap)
        if (environmentState == null) {
            logger.info("ENVIRONMENT_STATE datasource not found")
            return null
        }
        return environmentState.endpointsList.firstOrNull { it.id == clientInfo.deviceId }
            .let { endpoint ->
                endpoint?.capabilitiesList
                    ?.firstOrNull { capability -> capability.`is`(CapabilityProto.TVideoCallCapability::class.java) }
                    ?.let { capability ->
                        videoCallCapabilityConverter.convert(
                            capability.unpack(
                                CapabilityProto.TVideoCallCapability::class.java
                            )
                        )
                    }
            }
    }

    private fun getEnvironmentState(dataSourcesMap: Map<Int, TDataSource>): EnvironmentStateProto.TEnvironmentState? {
        val environmentStateDataSourceO = dataSourcesMap[DataSourceType.EDataSourceType.ENVIRONMENT_STATE.number]
        return if (environmentStateDataSourceO?.hasEnvironmentState() == true) {
            environmentStateDataSourceO.environmentState
        } else null
    }

    private fun getMusic(deviceState: TDeviceState): MegaMindRequest.Music? {
        val music = deviceState.music
        if (music == music.defaultInstanceForType) {
            return null
        }
        val mmPlayer = music.player
        val player: Player? = if (mmPlayer == mmPlayer.defaultInstanceForType) {
            null
        } else {
            Player(mmPlayer.pause)
        }
        val mmCurrentlyPlaying = music.currentlyPlaying
        val currentlyPlaying = if (mmCurrentlyPlaying != mmCurrentlyPlaying.defaultInstanceForType) {
            MegaMindRequest.CurrentlyPlaying(timestampToInstant(mmCurrentlyPlaying.lastPlayTimestamp))
        } else null

        val lastPlayTimestamp = music.lastPlayTimestamp
        val lastStopTimestamp = music.lastStopTimestamp
        return MegaMindRequest.Music(
            player,
            currentlyPlaying,
            timestampToInstant(lastPlayTimestamp),
            timestampToInstant(lastStopTimestamp)
        )
    }

    private fun timestampToInstant(timestamp: Double): Instant? {
        return if (timestamp == 0.0) {
            null
        } else Instant.ofEpochMilli(timestamp.toLong())
    }

    private fun getVideo(deviceState: TDeviceState): MegaMindRequest.Video? {
        val video = deviceState.video
        if (video == video.defaultInstanceForType) {
            return null
        }
        val mmCurrentlyPlaying = video.currentlyPlaying
        val currentlyPlaying = if (mmCurrentlyPlaying != mmCurrentlyPlaying.defaultInstanceForType) {
            MegaMindRequest.CurrentlyPlaying(timestampToInstant(mmCurrentlyPlaying.lastPlayTimestamp))
        } else null
        return MegaMindRequest.Video(
            currentlyPlaying,
            timestampToInstant(video.lastPlayTimestamp),
            timestampToInstant(video.lastStopTimestamp)
        )
    }

    private fun getRadio(deviceState: TDeviceState): Radio? {
        val radio = deviceState.radio
        if (radio.fieldsMap.isEmpty()) {
            return null
        }
        val lastPlayTimestampO =
            radio.fieldsMap?.get("currently_playing")?.structValue?.fieldsMap?.get("last_play_timestamp")?.numberValue
        val currentlyPlaying = lastPlayTimestampO?.let { MegaMindRequest.CurrentlyPlaying(timestampToInstant(it)) }

        val pause = radio.fieldsMap?.get("player")?.structValue?.fieldsMap?.get("pause")?.boolValue
        return Radio(Player(pause != false), currentlyPlaying)
    }

    private fun getAudioPlayer(
        deviceState: TDeviceState,
        request: TScenarioBaseRequest
    ): DeviceState.AudioPlayer? {
        val audioPlayer = deviceState.audioPlayer
        if (audioPlayer == audioPlayer.defaultInstanceForType) {
            return null
        }

        val playerState = mapPlayerState(audioPlayer.playerState)
        val mmCurrentlyPlaying = audioPlayer.currentlyPlaying

        val currentlyPlaying: MegaMindRequest.CurrentlyPlaying? =
            if (mmCurrentlyPlaying != mmCurrentlyPlaying.defaultInstanceForType) {
                MegaMindRequest.CurrentlyPlaying(timestampToInstant(mmCurrentlyPlaying.lastPlayTimestamp))
            } else null

        return DeviceState.AudioPlayer(
            audioPlayer.currentlyPlaying.streamId,
            audioPlayer.offsetMs.toLong(),
            playerState,
            audioPlayer.scenarioMetaMap,
            currentlyPlaying,
            timestampToInstant(audioPlayer.lastPlayTimestamp),
            getLastStopTimestamp(audioPlayer, request.serverTimeMs)
        )
    }

    // TODO: after SK-4545 - get property from device_state.audio_player.last_stop_timestamp
    private fun getLastStopTimestamp(
        audioPlayer: TAudioPlayer,
        serverTimeMs: Long
    ): Instant? {
        val lastStopTimestamp = timestampToInstant(audioPlayer.lastStopTimestamp)
        if (lastStopTimestamp !== null) return lastStopTimestamp

        val lastPlayTimestamp = timestampToInstant(audioPlayer.lastPlayTimestamp) ?: return null

        // Calculating last stop timestamp = last_play_timestamp + offset_ms - offset_ms_on_start
        // offset_ms_on_start get from scenario meta  - not precise - especially with rewinds
        val currOffsetMs = audioPlayer.offsetMs
        val offSetOnStartMs = audioPlayer.scenarioMetaMap[DeviceState.TRACK_OFFSET_MS_ON_START]?.toIntOrNull() ?: 0

        // approximate play time based on pings
        val playedMsApprox = audioPlayer.playedMs

        // can be negative on back rewinds
        val listenTimeProbablyMs =
            if (currOffsetMs - offSetOnStartMs > 0) currOffsetMs - offSetOnStartMs else playedMsApprox

        // last stop can be in future on forward rewind
        val lastPlayMs = lastPlayTimestamp.toEpochMilli()
        val lastStopProbablyMs = lastPlayMs +
            (if (lastPlayMs + listenTimeProbablyMs < serverTimeMs) listenTimeProbablyMs else playedMsApprox)
        return Instant.ofEpochMilli(lastStopProbablyMs)
    }

    private fun mapPlayerState(playerState: TPlayerState): AudioPlayerActivityState {
        return when (playerState) {
            TPlayerState.Idle -> AudioPlayerActivityState.IDLE
            TPlayerState.Playing -> AudioPlayerActivityState.PLAYING
            TPlayerState.Paused -> AudioPlayerActivityState.PAUSED
            TPlayerState.Buffering -> AudioPlayerActivityState.PLAYING
            TPlayerState.Stopped -> AudioPlayerActivityState.STOPPED
            TPlayerState.Finished -> AudioPlayerActivityState.FINISHED
            else -> AudioPlayerActivityState.IDLE
        }
    }

    private fun mapAge(age: EAge): Age {
        return when (age) {
            EAge.Adult -> Age.ADULT
            EAge.Child -> Age.CHILD
            else -> Age.ADULT
        }
    }

    private fun getLocation(baseRequest: TScenarioBaseRequest): LocationInfo? =
        baseRequest.location.takeIf { baseRequest.hasLocation() }
            ?.let { LocationInfo(it.lat, it.lon, it.accuracy) }

    private fun getUserRegion(dataSourcesMap: Map<Int, TDataSource>): UserRegion? {
        val userLocationDataSourceO = dataSourcesMap[DataSourceType.EDataSourceType.USER_LOCATION.number]
        return if (userLocationDataSourceO?.hasUserLocation() == true) {
            val userLocationProto = userLocationDataSourceO.userLocation
            UserRegion(regionId = userLocationProto.userRegion, countryId = userLocationProto.userCountry)
        } else null
    }

    private fun getIotUserInfo(dataSourcesMap: Map<Int, TDataSource>): IoT.TIoTUserInfo? {
        val iotUserInfoDataSourceO = dataSourcesMap[DataSourceType.EDataSourceType.IOT_USER_INFO.number]
        DataSourceType.EDataSourceType.CONTACTS_LIST.number
        return if (iotUserInfoDataSourceO?.hasIoTUserInfo() == true) {
            iotUserInfoDataSourceO.ioTUserInfo
        } else null
    }

    private fun getContactsList(dataSourcesMap: Map<Int, TDataSource>): ContactsList? {
        val contactsListDataSourceO = dataSourcesMap[DataSourceType.EDataSourceType.CONTACTS_LIST.number]
        return if (contactsListDataSourceO?.hasContactsList() == true) {
            contactsListConverter.convert(contactsListDataSourceO.contactsList)
        } else null
    }

    private fun getBlackboxInfo(dataSourcesMap: Map<Int, TDataSource>): BlackboxInfo? {
        val blackboxInfoO = dataSourcesMap[DataSourceType.EDataSourceType.BLACK_BOX.number]
        return if (blackboxInfoO?.hasUserInfo() == true) {
            val blackboxInfoProto = blackboxInfoO.userInfo
            BlackboxInfo(
                uid = blackboxInfoProto.uid,
                email = blackboxInfoProto.email,
                firstName = blackboxInfoProto.firstName,
                lastName = blackboxInfoProto.lastName,
                phone = blackboxInfoProto.phone,
                hasYandexPlus = blackboxInfoProto.hasYandexPlus,
                hasMusicSubscription = blackboxInfoProto.hasMusicSubscription,
                isBetaTester = blackboxInfoProto.isBetaTester,
                isStaff = blackboxInfoProto.isStaff,
            )
        } else null
    }

    private fun getDiscoveryGcCandidates(dataSourcesMap: Map<Int, TDataSource>): SkillDiscoveryCandidates? {
        val skillDiscoveryGcSaasCandidates = dataSourcesMap[DataSourceType.EDataSourceType.SKILL_DISCOVERY_GC.number]
        return if (skillDiscoveryGcSaasCandidates?.hasSkillDiscoveryGcSaasCandidates() == true) {
            val protoCandidates = skillDiscoveryGcSaasCandidates.skillDiscoveryGcSaasCandidates
            val candidates =
                protoCandidates.saasCandidateList.map { protoCandidate: RequestProto.TSkillDiscoverySaasCandidates.TSaasCandidate ->
                    SkillDiscoveryCandidate(protoCandidate.skillId, protoCandidate.relevance)
                }
            SkillDiscoveryCandidates(candidates = candidates)
        } else null
    }

    private fun getClientInfo(src: TScenarioBaseRequest): ClientInfo {
        val clientInfo = src.clientInfo
        val deviceId = clientInfo.deviceId
        return ClientInfo(
            uuid = clientInfo.uuid,
            platform = clientInfo.platform,
            osVersion = clientInfo.osVersion,
            appVersion = clientInfo.appVersion,
            appId = clientInfo.appId,
            deviceId = Strings.trimToNull(deviceId),
            deviceManufacturer = clientInfo.deviceManufacturer,
            deviceModel = clientInfo.deviceModel,
            lang = clientInfo.lang,
            timezone = clientInfo.timezone,
            interfaces = Interfaces.fromProto(src.interfaces)
        )
    }
}
