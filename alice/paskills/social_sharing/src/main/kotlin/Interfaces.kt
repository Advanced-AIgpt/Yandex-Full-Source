package ru.yandex.alice.social.sharing

import com.fasterxml.jackson.annotation.JsonValue
import ru.yandex.alice.social.sharing.document.InvalidParamsException


class InvalidInterfaceException(
    stringValue: String,
): InvalidParamsException()

enum class Interface(
    @JsonValue val value: String,
) {
    AUDIO_PLAYER("audio_player");

    companion object {

        private val valueToInterface = Interface.values()
            .map { it.value to it }
            .toMap()

        @Throws(InvalidInterfaceException::class)
        fun fromStringUnsafe(name: String): Interface {
            return valueToInterface[name] ?: throw InvalidInterfaceException(name)
        }
    }
}

internal object ClientFeatures {
    val AUDIO_CLIENT = "audio_client"
}

internal val INTERFACE_TO_REQUIRED_FEATURES: Map<Interface, List<String>> = mapOf(
    Interface.AUDIO_PLAYER to listOf(ClientFeatures.AUDIO_CLIENT)
)
