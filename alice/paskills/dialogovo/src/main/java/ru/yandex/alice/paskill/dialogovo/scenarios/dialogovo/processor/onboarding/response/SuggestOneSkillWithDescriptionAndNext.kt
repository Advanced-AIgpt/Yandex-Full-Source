package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response

import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillRecommendationsProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSuggestVoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillRecommendationsAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.SkillsOnboardingScrollNextDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.SkillsOnboardingDefinition

class SuggestOneSkillWithDescriptionAndNext(
    voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
    phrases: Phrases,
    phraseWithoutDescriptionKey: String,
    phraseWithDescriptionKey: String,
    private val phraseWithoutDescriptionOnNextKey: String,
    private val phraseWithDescriptionOnNextKey: String,
    skillProvider: SkillRecommendationsProvider,
    private val onboardingType: SkillsOnboardingDefinition.SkillsOnboardingType
) : SuggestOneSkillWithDescription(
    voiceButtonFactory,
    phrases,
    phraseWithoutDescriptionKey,
    phraseWithDescriptionKey,
    skillProvider
),
    SkillOnboardingWithNextSkillResponseGenerator {

    override fun generateActions(skill: SkillInfo) = mapOf(
        "confirm" to
            voiceButtonFactory.createConfirmSkillActivationVoiceButton(
                skill,
                ActivationSourceType.SKILLS_STATION_ONBOARDING
            ),
        "decline" to
            voiceButtonFactory.createDeclineDoNothingButton(),
        "next" to
            ActionRef.withCallback(
                SkillsOnboardingScrollNextDirective(onboardingType),
                // TODO: Switch to our grammar
                ActionRef.NluHint(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS_MORE)
            )
    )

    override fun generateResponseOnNext(
        context: Context,
        request: MegaMindRequest<DialogovoState>
    ): BaseRunResponse<DialogovoState> {
        // TODO: Remember previous in sequence showed skills instead of random
        val skills = skillProvider.getSkills(request.random, 1, request)
        if (skills.size == 0) {
            return DefaultIrrelevantResponse.create(
                Intents.IRRELEVANT, request.random,
                request.isVoiceSession()
            )
        }
        val skill = skills[0]
        context.analytics.setIntent(Intents.SKILLS_ONBOARDING_NEXT)
        context.analytics.addObject(SkillAnalyticsInfoObject(skill))
        context.analytics.addObject(
            SkillRecommendationsAnalyticsInfoObject(
                listOf(skill.id),
                RecommendationType.SKILLS_ONBOARDING
            )
        )
        val description = skill.editorDescription
        val nlg = if (description == null)
            phrases.getRandomTextWithTts(
                phraseWithoutDescriptionOnNextKey,
                request.random,
                listOf(skill.name),
                listOf(skill.getNameTts())
            ) else
            phrases.getRandomTextWithTts(
                phraseWithDescriptionOnNextKey,
                request.random,
                listOf(skill.name, description),
                listOf(skill.getNameTts(), description)
            )

        val voiceLayout = Layout.builder()
            .textCard(nlg.text)
            .outputSpeech(nlg.tts)
            .shouldListen(request.isVoiceSession())
            .build()

        return RunOnlyResponse(
            ScenarioResponseBody(
                voiceLayout,
                context.analytics.toAnalyticsInfo(),
                false,
                generateActions(skill)
            )
        )
    }
}
