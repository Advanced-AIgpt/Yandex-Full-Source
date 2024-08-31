package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillRecommendationsProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSuggestVoiceButtonFactory

open class SuggestOneSkillWithDescription(
    voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
    phrases: Phrases,
    private val phraseWithoutDescriptionKey: String,
    private val phraseWithDescriptionKey: String,
    skillProvider: SkillRecommendationsProvider
) : SuggestOneSkill(voiceButtonFactory, phrases, phraseWithoutDescriptionKey, skillProvider) {

    override fun generateNlg(
        request: MegaMindRequest<DialogovoState>,
        skill: SkillInfo
    ): TextWithTts {
        val description = skill.editorDescription
        return if (description == null)
            phrases.getRandomTextWithTts(
                phraseWithoutDescriptionKey,
                request.random,
                listOf(skill.name),
                listOf(skill.getNameTts())
            ) else
            phrases.getRandomTextWithTts(
                phraseWithDescriptionKey,
                request.random,
                listOf(skill.name, description),
                listOf(skill.getNameTts(), description)
            )
    }
}
