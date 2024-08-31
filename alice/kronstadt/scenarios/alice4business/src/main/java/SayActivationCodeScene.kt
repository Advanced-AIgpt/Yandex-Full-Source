package ru.yandex.alice.kronstadt.scenarios.alice4business

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.directive.MordoviaCommandDirective
import ru.yandex.alice.kronstadt.core.directive.MordoviaShowDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.kronstadt.scenarios.alice4business.SayActivationCodeScene.Arg

internal const val ALICE4BUSINESS_DEVICE_LOCK_LOCKED = "alice4business.device_lock.locked"
internal const val ALICE4BUSINESS_DEVICE_LOCK_LOCKED_SHORT = "alice4business.device_lock.locked.short"

@Component
open class SayActivationCodeScene(val phrases: Phrases) : AbstractScene<Any, Arg>("SAY_ACTIVATION_CODE", Arg::class) {

    data class Arg(
        val repeat: Boolean,
        val code: String? = null,
        val stationUrl: String? = null,
    )

    override fun render(request: MegaMindRequest<Any>, args: Arg): RelevantResponse<Any> {
        val activationCode = args.code ?: ACTIVATION_CODE_FALLBACK
        val stationUrl = args.stationUrl ?: ""
        val webviewOpen = isWebviewOpen(request)
        val shouldOpenWebview = !webviewOpen && stationUrl.isNotBlank()
        val outputSpeech = if (args.repeat)
            phrases.get("alice4business.device_lock.activation_code", "", prepareCodeForTTS(activationCode))
        else phrases.getRandom(
            "alice4business.device_lock.device_is_locked", request.random, prepareCodeForTTS(activationCode)
        )

        val directives = mutableListOf<MegaMindDirective>()
        if (webviewOpen) {
            directives.add(
                MordoviaCommandDirective(
                    WEBVIEW_COMMAND_SET_CODE,
                    mapOf("code" to activationCode),
                    WEBVIEW_VIEW_KEY
                )
            )
        }
        if (shouldOpenWebview) {
            directives.add(
                MordoviaShowDirective(
                    "$stationUrl?code=$activationCode",
                    true,
                    WEBVIEW_VIEW_KEY
                )
            )
        }
        val intent = if (args.repeat) {
            ALICE4BUSINESS_DEVICE_LOCK_LOCKED_SHORT
        } else {
            ALICE4BUSINESS_DEVICE_LOCK_LOCKED
        }
        return RunOnlyResponse(
            layout = Layout(
                outputSpeech = outputSpeech,
                shouldListen = false,
                directives = directives,

                ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent),
            isExpectsRequest = true,
        )
    }

    private val codeSplitter = Regex(".{1,2}")

    private fun prepareCodeForTTS(activationCode: String): String =
        codeSplitter.findAll(activationCode).map { it.value }.joinToString(separator = ", ")
}
