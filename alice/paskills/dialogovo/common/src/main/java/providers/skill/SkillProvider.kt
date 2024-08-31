package ru.yandex.alice.paskill.dialogovo.providers.skill

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import java.util.Optional

interface SkillProvider : SkillActivationPhraseSearcher {
    fun getSkill(skillId: String): Optional<SkillInfo>
    fun getSkillDraft(skillId: String): Optional<SkillInfo>
    fun getActivationIntentFormNames(skillIds: List<String>): Map<String, Set<String>>
    fun findAllSkills(): List<SkillInfo>
    fun getSkillsByTags(tagsKey: SkillTagsKey): List<SkillInfo>
    fun getSkillsByCategory(categoryKey: SkillCategoryKey): List<SkillInfo>
    fun isReady(): Boolean
}
