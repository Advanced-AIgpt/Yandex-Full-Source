package ru.yandex.alice.kronstadt.scenarios.automotive

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene

private val AUTOMOTIVE_HVAC = ScenarioMeta("automotive_hvac", "AutomotiveHvac", "automotive_hvac")

const val AUTOMOTIVE_FRONT_DEFROSTER_OFF_FRAME_NAME = "alice.front_defroster_off"
const val AUTOMOTIVE_FRONT_DEFROSTER_ON_FRAME_NAME = "alice.front_defroster_on"
const val AUTOMOTIVE_REAR_DEFROSTER_OFF_FRAME_NAME = "alice.rear_defroster_off"
const val AUTOMOTIVE_REAR_DEFROSTER_ON_FRAME_NAME = "alice.rear_defroster_on"
const val AUTOMOTIVE_IRRELEVANT = "automotive.irrelevant"

private const val URI_HVAC = "yandexauto://hvac?action="
private const val TURNING_ON_TEXT = "Включаю"
private const val TURNING_OFF_TEXT = "Выключаю"
private const val IRRELEVANT_TEXT = "Я умею управлять климатом в автомобиле только в Яндекс Авто"


@Component
class AutomotiveHvacScenario : AbstractNoStateScenario(AUTOMOTIVE_HVAC) {

    override val irrelevantResponseFactory: IrrelevantResponse.Factory<Any> =
        AutomotiveIrrelevantResponse.Factory()

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? =
        request.handle {
            onClient(ClientInfo::isYaAuto) {
                onFrame(AUTOMOTIVE_FRONT_DEFROSTER_OFF_FRAME_NAME) { frame ->
                    scene<TurnFrontDefrosterOff>()
                }
                onFrame(AUTOMOTIVE_FRONT_DEFROSTER_ON_FRAME_NAME) { frame ->
                    scene<TurnFrontDefrosterOn>()
                }
                onFrame(AUTOMOTIVE_REAR_DEFROSTER_OFF_FRAME_NAME) { frame ->
                    scene<TurnRearDefrosterOff>()
                }
                onFrame(AUTOMOTIVE_REAR_DEFROSTER_ON_FRAME_NAME) { frame ->
                    scene<TurnRearDefrosterOn>()
                }
            }
        }
}

@Component
object TurnFrontDefrosterOff : AbstractNoargScene<Any>("TURN_FRONT_DEFROSTER_OFF") {
    private val logger = LogManager.getLogger()

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        logger.info("Process TurnFrontDefrosterOff")

        return createResponse(
            text = TURNING_OFF_TEXT,
            uri = URI_HVAC + "front_defroster_off",
            intent = AUTOMOTIVE_FRONT_DEFROSTER_OFF_FRAME_NAME
        )
    }
}

@Component
object TurnFrontDefrosterOn : AbstractNoargScene<Any>("TURN_FRONT_DEFROSTER_ON") {
    private val logger = LogManager.getLogger()

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        logger.info("Process TurnFrontDefrosterOn")

        return createResponse(
            text = TURNING_ON_TEXT,
            uri = URI_HVAC + "front_defroster_on",
            intent = AUTOMOTIVE_FRONT_DEFROSTER_ON_FRAME_NAME
        )
    }
}

@Component
object TurnRearDefrosterOff : AbstractNoargScene<Any>("TURN_REAR_DEFROSTER_OFF") {
    private val logger = LogManager.getLogger()

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        logger.info("Process TurnRearDefrosterOff")

        return createResponse(
            text = TURNING_OFF_TEXT,
            uri = URI_HVAC + "rear_defroster_off",
            intent = AUTOMOTIVE_REAR_DEFROSTER_OFF_FRAME_NAME
        )
    }
}

@Component
object TurnRearDefrosterOn : AbstractNoargScene<Any>("TURN_REAR_DEFROSTER_ON") {
    private val logger = LogManager.getLogger()

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        logger.info("Process TurnRearDefrosterOn")

        return createResponse(
            text = TURNING_ON_TEXT,
            uri = URI_HVAC + "rear_defroster_on",
            intent = AUTOMOTIVE_REAR_DEFROSTER_ON_FRAME_NAME
        )
    }
}

private fun createResponse(text: String, uri: String?, intent: String): RunOnlyResponse<Any> {
    return RunOnlyResponse(
        layout = Layout.textWithOutputSpeech(
            textWithTts = TextWithTts(text),
            directives = if (uri != null) mutableListOf(OpenUriDirective(uri)) else emptyList()
        ),
        analyticsInfo = AnalyticsInfo(intent = intent),
        state = null
    )
}

private class AutomotiveIrrelevantResponse<State>(responseBody: ScenarioResponseBody<State>) :
    IrrelevantResponse<State>(responseBody) {

    class Factory<State> : IrrelevantResponse.Factory<State> {

        override fun create(request: MegaMindRequest<State>): IrrelevantResponse<State> {
            return AutomotiveIrrelevantResponse(
                ScenarioResponseBody(
                    layout = Layout.textLayout(
                        text = IRRELEVANT_TEXT,
                        outputSpeech = IRRELEVANT_TEXT
                    ),
                    analyticsInfo = AnalyticsInfo(AUTOMOTIVE_IRRELEVANT),
                    state = null
                )
            )
        }
    }
}
