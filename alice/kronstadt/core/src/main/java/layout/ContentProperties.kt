package ru.yandex.alice.kronstadt.core.layout

data class ContentProperties(
    val containsSensitiveDataInRequest: Boolean = false,
    val containsSensitiveDataInResponse: Boolean = false
) {
    companion object {
        @JvmStatic
        fun sensitiveRequest() = ContentProperties(containsSensitiveDataInRequest = true)

        @JvmStatic
        fun sensitiveResponse() = ContentProperties(containsSensitiveDataInResponse = true)

        @JvmStatic
        fun allSensitive() = ContentProperties(
            containsSensitiveDataInRequest = true,
            containsSensitiveDataInResponse = true
        )
    }
}