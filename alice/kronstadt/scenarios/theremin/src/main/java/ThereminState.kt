package ru.yandex.alice.paskill.dialogovo.scenarios.theremin


data class ThereminState(
    val isInternal: Boolean,
    // skillId for external modes
    val modeId: String? = null
)

