package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor
import ru.yandex.alice.paskill.dialogovo.providers.UserProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.UserAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.ReturnToDeviceCallbackDirective
import ru.yandex.alice.paskill.dialogovo.service.xiva.XivaService
import java.util.Optional

abstract class ReturnToStationProcessor<DirectiveType : ReturnToDeviceCallbackDirective>(
    protected val skillApplier: MegaMindRequestSkillApplier,
    protected val skillProvider: SkillProvider,
    protected val xivaService: XivaService,
    protected val requestContext: RequestContext,
    protected val phrases: Phrases,
    protected val userProvider: UserProvider,
    private val analyticsInfoAction: AnalyticsInfoAction,
    private val errorAnalyticsAction: AnalyticsInfoAction,
) : ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {

    protected abstract val directiveClass: Class<DirectiveType>
    protected abstract val intent: String

    protected abstract val nlgKey: String
    protected abstract val nlgErrorKey: String

    protected abstract fun removeInitialDeviceId(originalDirective: DirectiveType): DirectiveType

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return onCallbackDirective(directiveClass).test(request)
    }

    override val applyArgsType: Class<RequestSkillApplyArguments> = RequestSkillApplyArguments::class.java

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val skillId = request
            .input
            .getDirective(directiveClass)
            .skillId
        return ApplyNeededResponse<DialogovoState>(
            RequestSkillApplyArguments.create(
                skillId,
                Optional.empty(),
                Optional.empty(),
                Optional.empty()
            )
        )
    }

    override fun apply(
        request: MegaMindRequest<DialogovoState>,
        context: Context,
        applyArguments: RequestSkillApplyArguments
    ): ScenarioResponseBody<DialogovoState> {
        val directive = request.input.getDirective(directiveClass)
        if (directive.initialDeviceId == null) {
            return skillApplier.processApply(request, context, applyArguments, Optional.empty())
        } else {
            // for station responses, can be moved to commit
            val skill = skillProvider.getSkill(applyArguments.skillId)
                .orElseThrow {
                    AliceHandledException(
                        "Skill not found " + applyArguments.skillId,
                        ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                        "Навык не найден",
                        "Навык не найден"
                    )
                }
            context.analytics.setIntent(intent)
            context.analytics.addObject(SkillAnalyticsInfoObject(skill))
            context.analytics.addObject(
                UserAnalyticsInfoObject(
                    // application_id
                    userProvider.getApplicationId(skill, request.clientInfo.uuid),
                    userProvider.getPersistentUserId(skill, requestContext.currentUserId),
                )
            )
            context.analytics.addAction(analyticsInfoAction)
            return sendPushToInitialDevice(context, skill, request, directive)
        }
    }

    protected open fun sendPushToInitialDevice(
        ctx: Context,
        skill: SkillInfo,
        request: MegaMindRequest<DialogovoState>,
        directive: DirectiveType,
    ): ScenarioResponseBody<DialogovoState> {
        val state = request.getStateO().orElse(null)
        return try {
            val callbackPayload = removeInitialDeviceId(directive)
            xivaService.sendCallbackDirectiveAsync(
                requestContext.currentUserId,
                directive.initialDeviceId!!,
                callbackPayload
            )
            val text: String = phrases.getRandom(nlgKey, request.random)
            val layout = Layout.silentText(text)
            ScenarioResponseBody(layout, state, ctx.analytics.toAnalyticsInfo(), true)
        } catch (e: Exception) {
            val text: String = phrases.getRandom(nlgErrorKey, request.random)
            throw AliceHandledException(
                message = "Unable to handle return to device callback in processor $this",
                action = errorAnalyticsAction,
                aliceText = text,
                aliceSpeech = text,
                expectRequest = true,
                cause = e
            )
        }
    }
}
