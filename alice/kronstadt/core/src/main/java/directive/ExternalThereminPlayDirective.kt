package ru.yandex.alice.kronstadt.core.directive

data class ExternalThereminPlayDirective(
    val skillId: String,
    val repeatSoundInside: Boolean,
    val noOverlaySamples: Boolean,
    val stopOnCeil: Boolean,
    val sampleUrls: List<String>
) : ThereminPlayDirective {
    override fun modeId(): String {
        return skillId
    }

    override fun isInternal(): Boolean {
        return false
    }
}
