package ru.yandex.alice.paskill.dialogovo.providers.skill

interface SkillActivationPhraseSearcher {
    /**
     * search skills by their activation phrases
     *
     * @param phrases set of trimmed nonempty strings of activation phrases
     * @return map phrase by skill id
     */
    // returns skill ID
    fun findSkillsByPhrases(phrases: Set<String>): Map<String, List<String>>
}
