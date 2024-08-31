package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import java.util.Random

// taken from https://a.yandex-team.ru/arc/trunk/arcadia/alice/vins/apps/personal_assistant/personal_assistant
// /config/scenarios/intents/common.nlg?rev=4728486#L71
val PREFIXES = listOf(
    "По вашему запросу ",
    "К сожалению, "
)

val PHRASES = listOf(
    "я ничего не нашла.",
    "ничего не нашлось.",
    "не получилось ничего найти.",
    "ничего найти не получилось."
)

class DefaultIrrelevantResponse<State>
private constructor(responseBody: ScenarioResponseBody<State>) : IrrelevantResponse<State>(responseBody) {

    class Factory<State>(val intent: String = IRRELEVANT) :
        IrrelevantResponse.Factory<State> {

        override fun create(request: MegaMindRequest<State>): IrrelevantResponse<State> =
            DefaultIrrelevantResponse.create(
                intent = intent,
                random = request.random,
                useVoice = request.isVoiceSession(),
            )
    }

    companion object {

        @JvmStatic
        fun <State> create(intent: String, random: Random, useVoice: Boolean): DefaultIrrelevantResponse<State> {
            return DefaultIrrelevantResponse(generateBody(intent, random, useVoice))
        }
    }
}

private fun <State> generateBody(intent: String, random: Random, useVoice: Boolean): ScenarioResponseBody<State> {
    val layout = generateLayout(random, useVoice)
    val analyticsInfo = AnalyticsInfo(intent, emptyList(), emptyList(), emptyList())
    return ScenarioResponseBody(
        layout = layout,
        analyticsInfo = analyticsInfo,
        isExpectsRequest = false
    )
}

private fun generateLayout(random: Random, useVoice: Boolean): Layout {
    val text = generateText(random)
    return Layout.textLayout(
        texts = listOf(text),
        outputSpeech = if (useVoice) text else null,
        shouldListen = true
    )
}

private fun generateText(random: Random): String {
    val prefix = PREFIXES[random.nextInt(PREFIXES.size)]
    val phrase = PHRASES[random.nextInt(PHRASES.size)]
    return prefix + phrase
}
