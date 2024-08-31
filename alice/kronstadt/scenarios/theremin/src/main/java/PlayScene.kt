package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.scheduling.annotation.Scheduled
import org.springframework.stereotype.Component
import org.springframework.util.StringUtils
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.ExternalThereminPlayDirective
import ru.yandex.alice.kronstadt.core.directive.InternalThereminPlayDirective
import ru.yandex.alice.kronstadt.core.directive.ThereminPlayDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.builder
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.nlu.libs.fstnormalizer.FstNormalizer
import ru.yandex.alice.nlu.libs.fstnormalizer.Lang
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminPacksConfig.PackConfig
import kotlin.random.Random

@Component
open class PlayScene(
    private val thereminSkillsDao: ThereminSkillsDao,
    private val requestContext: RequestContext,
    private val packsConfig: ThereminPacksConfig,
    @Value("\${thereminConfig.baseS3Url}")
    private val baseS3Url: String,
    @Value("\${fillCachesOnStartUp}") fillCachesOnStartUp: Boolean,
) : AbstractScene<ThereminState, PlayArgument>("THEREMIN_PLAY", PlayArgument::class) {

    private val basePath: String = baseS3Url + THEREMIN_SOUNDS_S3_PATH
    private var publicSkillsCache: Map<String, ThereminSkillInfoDB> = mapOf()
    private val normalizationService = FstNormalizer()

    val ONBOARDING_GUIDE = "Включаю режим синтезатора. Здесь вы можете сами создавать музыку. Начнём?" +
        " Просто скажите «Дай звук %s» или «Дай звук номер %s» и проведите рукой над колонкой."
    val NOT_INSTRUMENT_RESPONSES = listOf(
        "Увы, я не знаю такого звука. Выберите что-нибудь другое, и сыграем. Скажите, «Дай звук фортепиано» или " +
            "«Дай звук женский вокал».",
        "Это точно звук? Я такого не знаю. Зато могу включить что-нибудь другое: скажите, «Дай звук гитары» или " +
            "«Дай звук скрипки»."
    )

    // TODO: fix number instruments
    val OUT_OF_RANGE_INDEX_RESPONSES = listOf(
        "Пока я знаю всего " + Instrument.size() + " звуков. Но это пока.",
        "Моя библиотека пока не настолько большая. Но интересная! Я знаю " + Instrument.size() + " разных звуков."
    )
    val FULL_START = listOf(
        "Включаю. Проведите рукой над колонкой.",
        "Включаю. Чтобы выйти, скажите \"хватит\"."
    )
    val SHORT_START = "Включаю"
    val STATION_DISCLAIMER = listOf(
        "Режим синтезатора работает только в Яндекс.Станции Мини. Увы. Если не хотите сидеть в тишине, попросите " +
            "меня включить Яндекс.Музыку.",
        "Я умею включать разные звуки только в Яндекс.Станции Мини.",
        "Я умею включать разные звуки только в Яндекс.Станции Мини. Это мой новый дом. Как Яндекс.Станция, только" +
            " маленькая."
    )
    val TOO_FEW_UGC_SOUNDS = listOf(
        "Для игры на этом инструменте необходимо загрузить не менее двух звуков."
    )
    private val logger = LogManager.getLogger()

    init {
        if (fillCachesOnStartUp) {
            refreshCache()
        }
    }

    @Volatile
    private var ready = false

    val isReady: Boolean
        get() = ready

    override fun render(request: MegaMindRequest<ThereminState>, args: PlayArgument) =
        processMiniRequest(request, args)

    @Scheduled(fixedDelay = 300000, initialDelay = 300000)
    fun refreshCache() {
        publicSkillsCache = thereminSkillsDao.findThereminAllPublicSkills()
            .flatMap { skill ->
                skill.inflectedActivationPhrases
                    .map { phrase -> normalizationService.normalize(Lang.RUS, phrase) to skill }
            }
            .toMap()

        if (!ready) {
            ready = true
        }
    }

    internal fun processMiniRequest(
        baseRequest: MegaMindRequest<ThereminState>,
        thereminPlayFrame: SemanticFrame
    ): RelevantResponse<ThereminState> {
        return processMiniRequest(baseRequest, thereminPlayFrame.asPlayArgument())
    }

    fun processMiniRequest(
        baseRequest: MegaMindRequest<ThereminState>,
        argument: PlayArgument
    ): RelevantResponse<ThereminState> {
        if (argument.mielophone) {
            return createInternalThereminPlayResponse(baseRequest, Mielophone)
        } else if (argument.beatNumber != null) {

            return Instrument.byIndex(argument.beatNumber)
                ?.let { mode -> createInternalThereminPlayResponse(baseRequest, mode) }
                ?: indexOutOfRangeResponse()
        } else if (argument.beatName != null) {

            return Instrument.byName(argument.beatName)
                ?.let { possibleInstrument -> createInternalThereminPlayResponse(baseRequest, possibleInstrument) }
                ?: instrumentNotFoundResponse(baseRequest.krandom)
        } else if (argument.beatGroup != null) {

            val groupOptional = InstrumentGroup.byName(argument.beatGroup)
            if (groupOptional == null) {
                return instrumentNotFoundResponse(baseRequest.krandom)
            } else {
                val instruments = groupOptional.instruments
                val instrument: Instrument = if (argument.beatGroupIndex != null) {
                    // TODO: say that we have 5 more guitars

                    val enumIndex: Int = argument.beatGroupIndex
                    if (enumIndex > instruments.size || enumIndex <= 0) {
                        return instrumentNotFoundResponse(baseRequest.krandom)
                    }
                    instruments[enumIndex - 1]
                } else {
                    instruments[0]
                }
                return createInternalThereminPlayResponse(baseRequest, instrument)
            }
        } else if (argument.beatText != null) {

            val name = argument.beatText
            // check if its a public UGC instrument
            val thereminSkill: ThereminSkillInfoDB? = findExternalThereminSkill(name)
                ?: publicSkillsCache[normalizationService.normalize(Lang.RUS, name)]
                    ?.also { skill -> logger.debug("Found public UGC theremin skill: {}", skill) }

            return when {
                thereminSkill == null -> instrumentNotFoundResponse(baseRequest.krandom)
                thereminSkill.sounds.size < 2 -> tooFewInstruments(baseRequest.krandom)
                else -> {
                    val instrument = UserInstrument(
                        thereminSkill.id.toString(),
                        thereminSkill.developerType,
                        !thereminSkill.hideInStore
                    )
                    val directive = getExternalThereminPlayDirective(thereminSkill)
                    createInternalThereminPlayResponse(baseRequest, instrument, directive)
                }
            }
        } else {
            val text = String.format(
                ONBOARDING_GUIDE,
                Instrument.values().random(baseRequest.krandom).suggest,
                baseRequest.random.nextInt(Instrument.size()) + 1
            )
            return createTextResponse(text, AnalyticsInfo(intent = THEREMIN_ONBOARDING_GUIDE))
        }
    }

    private fun getExternalThereminPlayDirective(thereminSkill: ThereminSkillInfoDB): ExternalThereminPlayDirective {
        val sounds = thereminSkill.sounds.map { (_, originalPath) ->
            baseS3Url + StringUtils.trimLeadingCharacter(originalPath, '/')
        }

        // we need to reverse the order in which sounds are stored in dev-console.
        // file names are GUIDs so we can't sort by value
        return ExternalThereminPlayDirective(
            skillId = thereminSkill.id.toString(),
            repeatSoundInside = thereminSkill.soundsSettings.repeatSoundInside,
            noOverlaySamples = thereminSkill.soundsSettings.noOverlaySamples,
            stopOnCeil = thereminSkill.soundsSettings.stopOnCeil,
            sampleUrls = sounds.asReversed(),
        )
    }

    private fun findExternalThereminSkill(slotText: String): ThereminSkillInfoDB? {
        val currentUserId = requestContext.currentUserId
        return if (currentUserId != null) {
            val usersSkills: List<ThereminSkillInfoDB> =
                thereminSkillsDao.findThereminPrivateSkillsByUser(currentUserId)
            logger.debug("Found UGC theremin skills: {}", usersSkills.toString())

            val text = slotText.lowercase().replace("ё", "e")
            usersSkills.firstOrNull { skill -> skill.inflectedActivationPhrases.contains(text) }
        } else {
            logger.debug("No user provided for UGC theremin request")
            null
        }
    }

    private fun instrumentNotFoundResponse(r: Random): RunOnlyResponse<ThereminState> {
        val text = NOT_INSTRUMENT_RESPONSES.random(r)
        return createTextResponse(text, AnalyticsInfo(intent = THEREMIN_NOT_INSTRUMENT))
    }

    private fun tooFewInstruments(r: Random): RunOnlyResponse<ThereminState> {
        val text = TOO_FEW_UGC_SOUNDS.random(r)
        val layout = Layout.textLayout(
            texts = listOf(text.replace("+", "")),
            outputSpeech = text,
            shouldListen = false,
        )
        return RunOnlyResponse(
            layout = layout,
            state = null,
            analyticsInfo = AnalyticsInfo(intent = THEREMIN_TOO_FEW_SOUNDS),
            isExpectsRequest = false,
        )
    }

    private fun indexOutOfRangeResponse(): RunOnlyResponse<ThereminState> {
        val text = OUT_OF_RANGE_INDEX_RESPONSES.random()
        return createTextResponse(text, AnalyticsInfo(intent = THEREMIN_OUT_OF_RANGE))
    }

    private fun createTextResponse(text: String, analyticsInfo: AnalyticsInfo): RunOnlyResponse<ThereminState> =
        RunOnlyResponse(
            layout = Layout.textLayout(
                texts = listOf(text.replace("+", "")),
                outputSpeech = text.replace("Яндекс.Станц", "Яндекс-Станц"),
                shouldListen = true,
            ),
            state = null,
            analyticsInfo = analyticsInfo,
            isExpectsRequest = true,
        )

    private fun createTextResponse(
        directive: ThereminPlayDirective, startText: String,
        analyticsInfo: AnalyticsInfo
    ): RelevantResponse<ThereminState> {
        val layout = builder()
            .textCard(startText.replace("+", ""))
            .outputSpeech(startText)
            .shouldListen(false)
            .directives(mutableListOf(directive))
            .build()
        val state = ThereminState(directive.isInternal(), directive.modeId())
        return RunOnlyResponse(layout = layout, state = state, analyticsInfo = analyticsInfo, isExpectsRequest = false)
    }

    private fun createInternalThereminPlayResponse(
        request: MegaMindRequest<ThereminState>,
        instrument: GenericInstument
    ): RelevantResponse<ThereminState> {
        return if (Mielophone != instrument) {
            packsConfig.getPack(instrument.index)
                ?.let { pack -> createInternalPackAsExternal(request, instrument, pack) }
                ?: instrumentNotFoundResponse(request.krandom)
        } else {
            val directive: ThereminPlayDirective = InternalThereminPlayDirective(instrument.index)
            createInternalThereminPlayResponse(request, instrument, directive)
        }
    }

    private fun createInternalPackAsExternal(
        request: MegaMindRequest<ThereminState>,
        instrument: GenericInstument,
        pack: PackConfig
    ): RelevantResponse<ThereminState> {
        val paths: List<String> = IntRange(1, pack.numberOfTracks)
            .map { ind -> basePath + pack.path + "/" + ind + ".mp3" }

        val directive: ThereminPlayDirective = ExternalThereminPlayDirective(
            instrument.index.toString(),
            pack.isRepeatSoundInside,
            pack.isNoOverlaySamples,
            pack.isStopOnCeil,
            paths
        )
        return createInternalThereminPlayResponse(request, instrument, directive)
    }

    private fun createInternalThereminPlayResponse(
        request: MegaMindRequest<ThereminState>,
        instrument: GenericInstument,
        directive: ThereminPlayDirective
    ): RelevantResponse<ThereminState> {
        val analyticsInfo: ThereminPlayAnalyticsInfo
        val text: String
        if (request.getStateO().isPresent) {
            analyticsInfo = ThereminPlayAnalyticsInfo(instrument, ThereminPlayAnalyticsInfo.StartPhraseType.Short)
            text = SHORT_START
        } else {
            analyticsInfo = ThereminPlayAnalyticsInfo(instrument, ThereminPlayAnalyticsInfo.StartPhraseType.Full)
            text = FULL_START.random(request.krandom)
        }
        return createTextResponse(directive, text, analyticsInfo)
    }

    companion object {
        const val THEREMIN_SOUNDS_S3_PATH = "theremin_sounds/"
    }
}
