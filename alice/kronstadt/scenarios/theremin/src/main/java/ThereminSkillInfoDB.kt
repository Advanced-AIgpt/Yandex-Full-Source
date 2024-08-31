package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import java.util.UUID

data class ThereminSkillInfoDB(
    val id: UUID,
    val userId: String,
    val name: String,
    val onAir: Boolean,
    val backendSettings: BackendSettings,
    val developerType: String,
    val hideInStore: Boolean,
    val inflectedActivationPhrases: List<String> = listOf(name.lowercase().replace("ё", "е"))
) {

    val sounds: List<Sound>
        get() = backendSettings.soundSet.sounds.filterNotNull()
    val soundsSettings: Settings
        get() = backendSettings.soundSet.settings

    data class BackendSettings(val soundSet: SoundSet)

    data class SoundSet(val sounds: List<Sound?>, val settings: Settings)

    data class Sound(val id: String, val originalPath: String)

    data class Settings(val noOverlaySamples: Boolean, val repeatSoundInside: Boolean, val stopOnCeil: Boolean)
}
