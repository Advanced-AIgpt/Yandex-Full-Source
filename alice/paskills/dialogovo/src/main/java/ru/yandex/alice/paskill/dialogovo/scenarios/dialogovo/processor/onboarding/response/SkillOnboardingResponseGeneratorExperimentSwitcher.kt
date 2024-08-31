package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response

import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

class SkillOnboardingResponseGeneratorExperimentSwitcher(
    private val suggestOneSkill: SuggestOneSkill,
    private val suggestOneSkillWithDescription: SuggestOneSkillWithDescription,
    private val suggestOneSkillWithDescriptionAndNext: SuggestOneSkillWithDescriptionAndNext
) : SkillOnboardingWithNextSkillResponseGenerator {

    override fun generateResponse(
        context: Context,
        request: MegaMindRequest<DialogovoState>
    ): BaseRunResponse<DialogovoState> {
        if (request.hasExperiment(Experiments.ONBOARDING_STATION_SHOW_SKILL_WITH_DESCRIPTION)) {
            return suggestOneSkillWithDescription.generateResponse(context, request)
        } else if (request.hasExperiment(Experiments.ONBOARDING_STATION_SHOW_SKILL_WITH_DESCRIPTION_AND_NEXT)) {
            return suggestOneSkillWithDescriptionAndNext.generateResponse(context, request)
        }
        return suggestOneSkill.generateResponse(context, request)
    }

    override fun generateResponseOnNext(
        context: Context,
        request: MegaMindRequest<DialogovoState>
    ): BaseRunResponse<DialogovoState> {
        return suggestOneSkillWithDescriptionAndNext.generateResponseOnNext(context, request)
    }
}
