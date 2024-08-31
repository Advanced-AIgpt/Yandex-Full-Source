package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import com.google.protobuf.Message
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.AbstractScenario
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import kotlin.random.asKotlinRandom

const val THEREMIN_PLAY = "theremin.play"
const val THEREMIN_ONBOARDING_GUIDE = "theremin.onboarding_guide"
const val THEREMIN_WHAT_IS_THEREMIN = "theremin.what_is_theremin"
const val THEREMIN_HOW_MANY_SOUNDS = "theremin.how_many_sounds"
const val THEREMIN_NOT_INSTRUMENT = "theremin.not_instrument"
const val THEREMIN_TOO_FEW_SOUNDS = "theremin.too_few_ogc_sounds_in_pack"
const val THEREMIN_OUT_OF_RANGE = "theremin.out_of_range"
const val THEREMIN_STATION_DISCLAIMER = "theremin.station_disclaimer"
const val THEREMIN_IRRELEVANT = "theremin.irrelevant"

const val THEREMIN_PLAY_SEMANTIC_FRAME_NAME = "alice.external_skill_theremin_play"
const val WHAT_IS_THEREMIN_SEMANTIC_FRAME_NAME = "alice.external_skill_what_is_theremin"
const val HOW_MANY_THEREMIN_SOUNDS_FRAME_NAME = "alice.external_skill_how_many_theremin_sounds"

val THEREMIN: ScenarioMeta = ScenarioMeta("theremin", "Theremin", "theremin")

@Component
open class ThereminScenario(
    val thereminStateConverter: ThereminStateConverter,
//    dialogovoMegaMindRequestListener: DialogovoMegaMindRequestListener,
) : AbstractScenario<ThereminState>(THEREMIN/*, listOf(dialogovoMegaMindRequestListener)*/) {
    override fun stateToProto(state: ThereminState, ctx: ToProtoContext): Message =
        thereminStateConverter.convert(state, ctx)

    override fun protoToState(request: RequestProto.TScenarioBaseRequest): ThereminState? =
        thereminStateConverter.convert(request)

    override fun selectScene(request: MegaMindRequest<ThereminState>) = request.handle {
        onClient(ClientInfo::isStationMini) {
            onFrame(THEREMIN_PLAY_SEMANTIC_FRAME_NAME) { frame ->
                sceneWithArgs(PlayScene::class, frame.asPlayArgument())
            }

            onFrame(HOW_MANY_THEREMIN_SOUNDS_FRAME_NAME) {
                scene<HowManeSoundsScene>()
            }

            onFrame(WHAT_IS_THEREMIN_SEMANTIC_FRAME_NAME) {
                scene<WhatIsTheremin>()
            }
        }

        onFrame(THEREMIN_PLAY_SEMANTIC_FRAME_NAME) {
            scene<StationDisclaimerScene>()
        }

    }
}

fun SemanticFrame.asPlayArgument(): PlayArgument {
    val slots: Map<String, String?> = this.slots.associate { it.name to it.value }

    return PlayArgument(
        mielophone = !slots["mielophone"].isNullOrBlank(),
        beatNumber = slots["beat_number"]?.toIntOrNull(),
        beatName = slots["beat_enum"],
        beatGroup = slots["beat_group"],
        beatGroupIndex = slots["beat_group_index"]?.toIntOrNull(),
        beatText = slots["beat_text"]?.takeIf { it.isNotBlank() },
    )
}

@Component
object StationDisclaimerScene : AbstractNoargScene<ThereminState>("THEREMIN_PLAY_DISCLAIMER") {
    private val logger = LogManager.getLogger()
    val STATION_DISCLAIMER = listOf(
        "Режим синтезатора работает только в Яндекс.Станции Мини. Увы. Если не хотите сидеть в тишине, попросите " +
            "меня включить Яндекс.Музыку.",
        "Я умею включать разные звуки только в Яндекс.Станции Мини.",
        "Я умею включать разные звуки только в Яндекс.Станции Мини. Это мой новый дом. Как Яндекс.Станция, только" +
            " маленькая."
    )

    override fun render(request: MegaMindRequest<ThereminState>): RelevantResponse<ThereminState> {
        logger.info("Process theremin play")

        val text = STATION_DISCLAIMER.random(request.random.asKotlinRandom())

        return RunOnlyResponse(
            layout = Layout.textLayout(
                texts = listOf(text),
                outputSpeech = text,
                shouldListen = request.isVoiceSession(),
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = THEREMIN_STATION_DISCLAIMER),
            isExpectsRequest = false,
        )
    }
}

@Component
object HowManeSoundsScene : AbstractNoargScene<ThereminState>("HOW_MANY_THEREMIN_SOUNDS") {
    private val logger = LogManager.getLogger()
    val HOW_MANY_SOUNDS = listOf(
        "В моей библиотеке %s звуков. И будет ещё больше! Скажите «Дай звук %s » или «Дай звук номер %s».",
        "Я знаю %s звуков. Хотите послушать? Скажите «Дай звук %s» или «Дай звук номер %s»."
    )

    override fun render(request: MegaMindRequest<ThereminState>): RelevantResponse<ThereminState> {

        logger.info("Process HowManyThereminSounds")

        val text = String.format(
            HOW_MANY_SOUNDS.random(request.random.asKotlinRandom()),
            Instrument.size(),
            Instrument.values().random(request.random.asKotlinRandom()).suggest,
            request.random.nextInt(Instrument.size()) + 1
        )
        return RunOnlyResponse(
            layout = Layout.textLayout(
                texts = listOf(prepareText(text)),
                outputSpeech = prepareTts(text),
                shouldListen = true,
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = THEREMIN_HOW_MANY_SOUNDS),
            isExpectsRequest = true,
        )
    }
}

@Component
object WhatIsTheremin : AbstractNoargScene<ThereminState>("WHAT_IS_THEREMIN") {
    private val logger = LogManager.getLogger()

    const val WHAT_IS_THEREMIN = "В Яндекс.Станции Мини вы можете играть мелодию движением рук+и. Для этого" +
        " просто скажите «Дай звук %s» или «Дай звук %s»."

    override fun render(request: MegaMindRequest<ThereminState>): RelevantResponse<ThereminState> {
        logger.info("Process WhatIsTheremin")

        val text = String.format(
            WHAT_IS_THEREMIN,
            Instrument.values().random(request.random.asKotlinRandom()).suggest,
            request.random.nextInt(Instrument.size()) + 1
        )

        return RunOnlyResponse(
            layout = Layout.textLayout(
                texts = listOf(prepareText(text)),
                outputSpeech = prepareTts(text),
                shouldListen = true,
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = THEREMIN_WHAT_IS_THEREMIN),
            isExpectsRequest = true,
        )
    }
}

private fun prepareText(text: String): String = text.replace("+", "")
private fun prepareTts(text: String): String = text.replace("Яндекс.Станц", "Яндекс-Станц")
