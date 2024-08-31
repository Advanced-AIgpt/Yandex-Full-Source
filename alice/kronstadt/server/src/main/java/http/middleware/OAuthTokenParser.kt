package ru.yandex.alice.kronstadt.server.http.middleware

import org.apache.logging.log4j.LogManager

object OAuthTokenParser {

    private val logger = LogManager.getLogger()

    fun parse(header: String?): String? {
        if (header == null) {
            return null
        }
        val parts = header.split(Regex("\\s+"), 2)
        if (parts.size == 1) {
            logger.error("Failed to parse OAuth header")
            return null
        } else {
            return parts[1]
        }
    }
}
