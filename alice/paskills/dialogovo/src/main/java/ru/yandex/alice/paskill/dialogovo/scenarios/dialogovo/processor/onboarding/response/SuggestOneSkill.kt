package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response

import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse.Companion.create
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.builder
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillRecommendationsProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSuggestVoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillRecommendationsAnalyticsInfoObject

open class SuggestOneSkill(
    protected val voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
    protected val phrases: Phrases,
    private val phraseKey: String,
    protected val skillProvider: SkillRecommendationsProvider
) : SkillOnboardingStationResponseGenerator {

    override fun generateResponse(
        context: Context,
        request: MegaMindRequest<DialogovoState>
    ): BaseRunResponse<DialogovoState> {
        val skills = skillProvider.getSkills(request.random, 1, request)
        if (skills.size == 0) {
            return create(
                Intents.IRRELEVANT, request.random,
                request.isVoiceSession()
            )
        }
        val skill = skills[0]
        context.analytics.setIntent(Intents.SKILLS_ONBOARDING_SHOW)
        context.analytics.addObject(SkillAnalyticsInfoObject(skill))
        context.analytics.addObject(
            SkillRecommendationsAnalyticsInfoObject(
                listOf(skill.id),
                RecommendationType.SKILLS_ONBOARDING
            )
        )
        val nlg = generateNlg(request, skill)

        val voiceLayout = builder()
            .textCard(nlg.text)
            .outputSpeech(nlg.tts)
            .shouldListen(request.isVoiceSession())
            .build()

        return RunOnlyResponse(
            layout = voiceLayout,
            state = null,
            analyticsInfo = context.analytics.toAnalyticsInfo(),
            isExpectsRequest = false,
            actions = generateActions(skill)
        )
    }

    open fun generateActions(skill: SkillInfo): Map<String, ActionRef> = mapOf(
        "confirm" to
            voiceButtonFactory.createConfirmSkillActivationVoiceButton(
                skill,
                ActivationSourceType.SKILLS_STATION_ONBOARDING
            ),
        "decline" to
            voiceButtonFactory.createDeclineDoNothingButton()
    )

    open fun generateNlg(
        request: MegaMindRequest<DialogovoState>,
        skill: SkillInfo
    ): TextWithTts {
        return phrases.getRandomTextWithTts(
            phraseKey,
            request.random,
            listOf(skill.name),
            listOf(skill.getNameTts())
        )
    }
}
