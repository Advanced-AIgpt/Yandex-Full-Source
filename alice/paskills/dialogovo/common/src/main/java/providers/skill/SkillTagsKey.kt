package ru.yandex.alice.paskill.dialogovo.providers.skill

enum class SkillTagsKey(val tags: Set<String>) {
    KIDS_GAMES_ONBOARDING(setOf("kids_games_onboarding")),
    TOP_UP_PHONE_PROVIDER(setOf("top_up_phone_provider"));

    companion object {
        @JvmStatic
        fun keyByTags(tags: Set<String>): List<SkillTagsKey> =
            enumValues<SkillTagsKey>().filter { tags.containsAll(it.tags) }
    }
}
