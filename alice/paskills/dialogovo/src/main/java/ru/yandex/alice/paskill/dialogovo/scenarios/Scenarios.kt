package ru.yandex.alice.paskill.dialogovo.scenarios

import ru.yandex.alice.kronstadt.core.ScenarioMeta

class Scenarios private constructor() {
    init {
        throw UnsupportedOperationException()
    }

    companion object {
        @JvmField
        val DIALOGOVO = ScenarioMeta("dialogovo", "Dialogovo", "dialogovo", "dialogovo", hashSetOf("megamind"))

        @JvmField
        val FLASHBRIEFINGS = ScenarioMeta(
            "flashbriefings", "ExternalSkillFlashBriefing", "external_skill_flash_briefing",
            "flash_briefing", hashSetOf("news")
        )

        @JvmField
        val RECIPES = ScenarioMeta("recipes", "ExternalSkillRecipes", "external_skill_recipes")

        @JvmField
        val DIALOGOVO_B2B = ScenarioMeta("dialogovo_b2b", "DialogovoB2b", "dialogovo_b2b")
    }
}
