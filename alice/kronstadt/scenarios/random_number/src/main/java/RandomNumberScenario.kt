package ru.yandex.alice.kronstadt.scenarios.rundom_number

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene

private val RANDOM_NUMBER = ScenarioMeta("random_number", "RandomNumber", "random_number")

private const val DEFAULT_MIN = 1
private const val DEFAULT_MAX = 100

@Component
class RandomNumberScenario : AbstractNoStateScenario(RANDOM_NUMBER) {

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? =
        request.handle {

            onFrame("alice.random_number") { frame ->
                sceneWithArgs(
                    RandomRender::class, RandomRender.Args(
                        lower = frame.getFirstSlot("lower_bound")?.value?.toIntOrNull(),
                        upper = frame.getFirstSlot("upper_bound")?.value?.toIntOrNull(),
                    )
                )
            }
        }
}

@Component
object RandomRender : AbstractScene<Any, RandomRender.Args>("random_number_render", Args::class) {
    data class Args(val lower: Int?, val upper: Int?)

    private val phrases = listOf(
        "Число %d.",
        "%d.",
        "Выпало число %d.",
    )

    override fun render(request: MegaMindRequest<Any>, args: Args): RelevantResponse<Any> {

        var lower = args.lower ?: DEFAULT_MIN
        var upper = args.upper ?: DEFAULT_MAX
        if (lower > upper) {
            lower = upper.also { upper = lower }
        }
        val value = request.random.nextInt(upper - lower + 1) + lower

        return renderAnswer(request, value)
    }

    private fun renderAnswer(
        request: MegaMindRequest<Any>,
        value: Int
    ): RunOnlyResponse<Any> {
        val text = phrases.random(request.krandom)
        return RunOnlyResponse(
            layout = Layout.textLayout(
                outputSpeech = "<speaker audio=\"rolling-dice.opus\"/>.sil<[100]>${text.format(value)}",
                text = text.format(value),
            ),
            state = null,
            analyticsInfo = AnalyticsInfo("random_number")
        )
    }
}
