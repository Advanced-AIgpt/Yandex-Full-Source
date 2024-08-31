package ru.yandex.alice.paskill.dialogovo.megamind.processor

interface ProcessorType {
    val name: String

    /**
     * true if can try lower priority processor on irrelevant answer from current
     */
    val isTryNextOnIrrelevant: Boolean
}
