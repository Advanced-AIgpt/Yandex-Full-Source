package ru.yandex.alice.kronstadt.core.domain

import java.util.Optional

data class QuasarAuxiliaryConfig(
    val alice4Business: Alice4BusinessConfig? = null
) {
    fun getAlice4BusinessO() = Optional.ofNullable(alice4Business)

    data class Alice4BusinessConfig(
        val preactivatedSkillIds: List<String> = listOf(),
        val unlocked: Boolean = false,
    ) {
        fun isUnlocked() = unlocked
    }
}
