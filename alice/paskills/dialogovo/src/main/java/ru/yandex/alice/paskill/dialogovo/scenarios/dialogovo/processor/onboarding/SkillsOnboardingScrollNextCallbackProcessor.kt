package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse.Companion.create
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SkillOnboardingAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.SkillsOnboardingScrollNextDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.SkillsOnboardingDefinition.SkillsOnboardingType
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response.SkillOnboardingWithNextSkillResponseGenerator
import java.util.function.Predicate

@Component
class SkillsOnboardingScrollNextCallbackProcessor(onboardingDefinitions: List<SkillsOnboardingDefinition>) :
    RunRequestProcessor<DialogovoState> {

    companion object {
        private val logger = LogManager.getLogger()
    }

    private val onboardingTypeToResponseGenerator: Map<SkillsOnboardingType, SkillOnboardingWithNextSkillResponseGenerator> =
        onboardingDefinitions
            .filter { it.responseGenerator is SkillOnboardingWithNextSkillResponseGenerator }
            .associate { definition -> definition.type to definition.responseGenerator as SkillOnboardingWithNextSkillResponseGenerator }

    override val type: RunRequestProcessorType = RunRequestProcessorType.SKILLS_ONBOARDING_GET_NEXT

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return onCallbackDirective(SkillsOnboardingScrollNextDirective::class.java)
            .and(Predicate.not(RunRequestProcessor.IS_IN_SKILL))
            .test(request)
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val (onboarding) = request.input.getDirective(
            SkillsOnboardingScrollNextDirective::class.java
        )
        logger.info("Continue with {} station skill onboarding", onboarding)
        val skillOnboardingWithNextSkillResponseGenerator = onboardingTypeToResponseGenerator[onboarding]
        if (skillOnboardingWithNextSkillResponseGenerator == null) {
            logger.error("Station skill onboarding on next skill generator not found by type {} ", onboarding)
            return create(
                Intents.IRRELEVANT, request.random,
                request.isVoiceSession()
            )
        }
        context.analytics.addObject(SkillOnboardingAnalyticsInfoObject(onboarding))
        return skillOnboardingWithNextSkillResponseGenerator.generateResponseOnNext(context, request)
    }
}
