package ru.yandex.alice.paskill.dialogovo.providers.recommendation

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import java.util.Optional

class SkillProviderWithRedefinedSkillIds(
    private val skillIdMapping: Map<String, String>,
    private val skillProvider: SkillProvider
) {
    fun getSkill(skillId: String): Optional<SkillInfo> {
        val skill = skillProvider.getSkill(skillIdMapping[skillId] ?: skillId)
        if (skill.isEmpty) {
            logger.info("Skill with id [{}] not found", skillId)
        }
        return skill
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
