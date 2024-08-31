package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData
import ru.yandex.alice.kronstadt.scenarios.video_call.PROVIDER_LOGIN_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL
import ru.yandex.alice.kronstadt.scenarios.video_call.cards.CustomTelegramDiv2Card
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallLoginDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallLoginDirective.TelegramStartLoginData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.VideoCallLoginFailedSemanticFrame
import java.util.UUID

private const val VIDEO_CALL_CONST_START_LOGIN_ID = "test_video_call_const_start_login_id"
private const val CONST_START_LOGIN_ID = "start_login_id"

@Component
object StartLoginScene
    : AbstractNoargScene<Any>(
    name = "start_login_scene"
) {
    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val startLoginId = if (!request.hasExperiment(VIDEO_CALL_CONST_START_LOGIN_ID))
            UUID.randomUUID().toString() else CONST_START_LOGIN_ID

        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = listOf(
                    getStartVideoCallLoginDirective(startLoginId),
                    getLoginShowView(startLoginId)
                )
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = PROVIDER_LOGIN_INTENT),
            isExpectsRequest = false
        )
    }

    private fun getStartVideoCallLoginDirective(startLoginId: String): StartVideoCallLoginDirective {
        val onFailSemanticFrame = VideoCallLoginFailedSemanticFrame
        val onFailCallback =  SemanticFrameRequestData(
            onFailSemanticFrame,
            SemanticFrameRequestData.AnalyticsTrackingModule(
                productScenario = VIDEO_CALL.productScenarioName,
                purpose = onFailSemanticFrame.defaultPurpose()
            )
        )
        return StartVideoCallLoginDirective(
            TelegramStartLoginData(
                id = startLoginId,
                onFailCallback = onFailCallback)
        )
    }

    private fun getLoginShowView(startLoginId: String) = ShowViewDirective(
        layer = ShowViewDirective.Layer.DIALOG,
        card = ShowViewDirective.Div2Card(CustomTelegramDiv2Card.card(
            logId = "telegram_login_screen",
            customType = "telegram_login",
            customProps = mapOf("id" to startLoginId),
            extension = "telegram-login",
        )),
        inactivityTimeout = ShowViewDirective.InactivityTimeout.INFINITY,
        doNotShowCloseButton = false
    )
}
