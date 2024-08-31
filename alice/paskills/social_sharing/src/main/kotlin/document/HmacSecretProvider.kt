package ru.yandex.alice.social.sharing.document

import java.util.*

interface HmacSecretProvider {

    fun getHmacSecret(skillId: String): ByteArray?

}

class EnvHmacSecretProvider: HmacSecretProvider {

    private val ENV_PREFIX = "PASKILLS_SOCIAL_SHARING_HMAC_"

    private val secrets: Map<String, ByteArray> = System.getenv()
        .filterKeys { key -> key.startsWith(ENV_PREFIX) }
        .mapKeys { entry -> entry.key.replace(ENV_PREFIX, "") }
        .mapValues { entry -> Base64.getDecoder().decode(entry.value) }

    override fun getHmacSecret(skillId: String): ByteArray? {
        return secrets[skillId]
    }

}
