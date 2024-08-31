package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType
import ru.yandex.alice.paskill.dialogovo.providers.UserProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.UserAgreementsAcceptedDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.UserAgreementsRejectedDirective
import ru.yandex.alice.paskill.dialogovo.service.xiva.XivaService

private object UserAgreementsAcceptedAction : AnalyticsInfoAction(
    "external_skill.user_agreements_accepted",
    "external_skill.account_linking_complete",
    "Пользователь принял пользовательские соглашения навыка, возврат в навык",
)

private object UserAgreementsRejectedAction : AnalyticsInfoAction(
    "external_skill.user_agreements_rejected",
    "external_skill.account_linking_complete",
    "Пользователь не принял пользовательские соглашения навыка, возврат в навык",
)

@Component
class UserAgreementAcceptedProcessor(
    skillApplier: MegaMindRequestSkillApplier,
    skillProvider: SkillProvider,
    xivaService: XivaService,
    requestContext: RequestContext,
    phrases: Phrases,
    userProvider: UserProvider,
) : ReturnToStationProcessor<UserAgreementsAcceptedDirective>(
    skillApplier,
    skillProvider,
    xivaService,
    requestContext,
    phrases,
    userProvider,
    UserAgreementsAcceptedAction,
    ErrorAnalyticsInfoAction.USER_AGREEMENT_CALLBACK_FAILURE,
) {
    override val directiveClass: Class<UserAgreementsAcceptedDirective> = UserAgreementsAcceptedDirective::class.java
    override val intent: String = Intents.EXTERNAL_SKILL_USER_AGREEMENTS_ACCEPTED
    override val nlgKey: String = "user_agreements.accepted"
    override val nlgErrorKey: String = "user_agreements.error"

    override val type: ProcessorType = RunRequestProcessorType.USER_AGREEMENTS_ACCEPTED

    override fun removeInitialDeviceId(
        originalDirective: UserAgreementsAcceptedDirective
    ): UserAgreementsAcceptedDirective {
        return originalDirective.copy(initialDeviceId = null)
    }
}

@Component
class UserAgreementRejectedProcessor(
    skillApplier: MegaMindRequestSkillApplier,
    skillProvider: SkillProvider,
    xivaService: XivaService,
    requestContext: RequestContext,
    phrases: Phrases,
    userProvider: UserProvider,
) : ReturnToStationProcessor<UserAgreementsRejectedDirective>(
    skillApplier,
    skillProvider,
    xivaService,
    requestContext,
    phrases,
    userProvider,
    UserAgreementsRejectedAction,
    ErrorAnalyticsInfoAction.USER_AGREEMENT_CALLBACK_FAILURE,
) {

    override val directiveClass: Class<UserAgreementsRejectedDirective> = UserAgreementsRejectedDirective::class.java
    override val intent: String = Intents.EXTERNAL_SKILL_USER_AGREEMENTS_REJECTED
    override val nlgKey: String = "user_agreements.rejected"
    override val nlgErrorKey: String = "user_agreements.error"

    override val type: ProcessorType = RunRequestProcessorType.USER_AGREEMENTS_REJECTED

    override fun removeInitialDeviceId(
        originalDirective: UserAgreementsRejectedDirective,
    ): UserAgreementsRejectedDirective {
        return originalDirective.copy(initialDeviceId = null)
    }
}
