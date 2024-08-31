package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

class RecipeClientInfo private constructor(
    @get:JvmName("isSupportsNativeTimers") val supportsNativeTimers: Boolean,
    val isSmartSpeaker: Boolean
) {

    companion object {
        @JvmField
        val SKILL = RecipeClientInfo(supportsNativeTimers = false, isSmartSpeaker = false)

        @JvmField
        val SMART_SPEAKER = RecipeClientInfo(supportsNativeTimers = true, isSmartSpeaker = true)

        @JvmField
        val NOT_SMART_SPEAKER = RecipeClientInfo(supportsNativeTimers = false, isSmartSpeaker = false)
    }
}
