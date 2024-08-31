package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg

enum class TtsTag {
    MASCULINE, FEMININE, NEUTER, DEFAULT;

    fun tag(): String {
        return when (this) {
            DEFAULT -> ""
            MASCULINE -> "#mas "
            FEMININE -> "#fem "
            NEUTER -> "#neu "
        }
    }
}
