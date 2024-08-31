package ru.yandex.alice.kronstadt.runner

import org.apache.logging.log4j.Logger
import org.springframework.core.env.ConfigurableEnvironment
import org.springframework.core.env.PropertySource
import org.springframework.core.env.StandardEnvironment
import org.springframework.core.log.LogMessage
import ru.yandex.alice.vault.VaultClient

class VaultPropertySource(
    name: String = VAULT_PROPERTY_SOURCE_NAME,
    client: VaultClient
) : PropertySource<VaultClient>(name, client) {

    override fun getProperty(name: String): String? {
        if (!name.startsWith(PREFIX)) {
            return null
        }
        logger.trace(LogMessage.format("Getting yav property for '%s'", name))
        return fetchVaultProperty(name.substring(PREFIX.length))
    }

    private fun fetchVaultProperty(property: String): String? {
        val matchResult = KEY_MATCHER.find(property)
        if (matchResult == null) {
            logger.warn("Incorrect Yav property: $property")
            return null
        }

        val version = matchResult.groups[1]!!
        if (!version.value.startsWith("ver-")) {
            logger.warn("Incorrect yav property. Key must stast with 'ver-'")
            return null
        }

        val key: String? = matchResult.groups[2]?.value

        if (key != null) {
            return source.getVersionKey(version.value, key).orElse(null)
        } else {
            return source.getVersion(version.value).version.value.firstOrNull()?.value
        }
    }

    companion object {
        private const val PREFIX = "yav."
        private const val VAULT_PROPERTY_SOURCE_NAME = "yav"
        private val KEY_MATCHER = "([-\\w]+)\\[([_\\w]+)]".toRegex()

        internal fun addToEnvironment(environment: ConfigurableEnvironment, logger: Logger) {
            val sources = environment.propertySources
            val existing = sources[VAULT_PROPERTY_SOURCE_NAME]
            if (existing != null) {
                logger.trace("VaultPropertySource already present")
                return
            }

            val oauthToken = environment.getProperty("YAV_OAUTH_TOKEN")?.takeUnless { it.isEmpty() }
            if (oauthToken.isNullOrEmpty()) {
                logger.info(
                    "\$YAV_OAUTH_TOKEN property not set. Skipping VaultPropertySource creation. " +
                        "You can get your Yav OAuth token here:\n" +
                        "https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982"
                )
                return
            }
            val vaultClient = VaultClient(oauthToken, VaultClient.Type.PROD)

            val vaultSource = VaultPropertySource(VAULT_PROPERTY_SOURCE_NAME, vaultClient)
            if (sources[StandardEnvironment.SYSTEM_ENVIRONMENT_PROPERTY_SOURCE_NAME] != null) {
                sources.addAfter(StandardEnvironment.SYSTEM_ENVIRONMENT_PROPERTY_SOURCE_NAME, vaultSource)
            } else {
                sources.addLast(vaultSource)
            }
            logger.trace("VaultPropertySource add to Environment")
        }
    }
}
