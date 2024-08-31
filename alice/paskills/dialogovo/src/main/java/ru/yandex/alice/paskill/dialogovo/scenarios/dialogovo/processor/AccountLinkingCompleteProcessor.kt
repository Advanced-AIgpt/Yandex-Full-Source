package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType
import ru.yandex.alice.paskill.dialogovo.providers.UserProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AccountLinkingCompleteAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.AccountLinkingCompleteDirective
import ru.yandex.alice.paskill.dialogovo.service.xiva.XivaService

@Component
class AccountLinkingCompleteProcessor(
    skillApplier: MegaMindRequestSkillApplier,
    skillProvider: SkillProvider,
    xivaService: XivaService,
    requestContext: RequestContext,
    phrases: Phrases,
    userProvider: UserProvider,
) : ReturnToStationProcessor<AccountLinkingCompleteDirective>(
    skillApplier,
    skillProvider,
    xivaService,
    requestContext,
    phrases,
    userProvider,
    AccountLinkingCompleteAction,
    ErrorAnalyticsInfoAction.ACCOUNT_LINKING_FAILURE,
) {
    override val directiveClass = AccountLinkingCompleteDirective::class.java
    override val intent = Intents.EXTERNAL_SKILL_ACC_LINKING_COMPLETE
    override val nlgKey = "account_linking.success"
    override val nlgErrorKey = "account_linking.error"

    override val type: ProcessorType = RunRequestProcessorType.ACCOUNT_LINKING_COMPLETE

    override fun removeInitialDeviceId(originalDirective: AccountLinkingCompleteDirective): AccountLinkingCompleteDirective {
        return originalDirective.copy(initialDeviceId = null)
    }
}
