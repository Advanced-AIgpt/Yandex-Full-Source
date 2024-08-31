package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonProperty

data class ThereminPacksConfig(val packs: Map<String, PackConfig>) {

    fun getPack(index: Int): PackConfig? = packs[index.toString()]

    data class PackConfig(
        val path: String,
        private val flags: Flags,
        val name: String,
        @field:JsonProperty("n_tracks") val numberOfTracks: Int
    ) {
        @get:JsonIgnore
        val isNoOverlaySamples: Boolean
            get() = flags.noOverlaySamples

        @get:JsonIgnore
        val isStopOnCeil: Boolean
            get() = flags.stopOnCeil

        @get:JsonIgnore
        val isRepeatSoundInside: Boolean
            get() = flags.repeatSoundInside
    }

    data class Flags(
        val noOverlaySamples: Boolean,
        val stopOnCeil: Boolean,
        val repeatSoundInside: Boolean
    )
}
