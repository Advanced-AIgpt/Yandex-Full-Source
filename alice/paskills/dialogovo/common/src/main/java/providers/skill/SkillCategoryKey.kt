package ru.yandex.alice.paskill.dialogovo.providers.skill

import ru.yandex.alice.paskill.dialogovo.domain.SkillCategories

enum class SkillCategoryKey(val category: String) {
    GAMES_TRIVIA_ACCESSORIES(SkillCategories.GAMES_TRIVIA_ACCESSORIES);

    companion object {
        @JvmStatic
        fun keyByCategory(category: String): SkillCategoryKey? =
            enumValues<SkillCategoryKey>().find { it.category == category }
    }
}
