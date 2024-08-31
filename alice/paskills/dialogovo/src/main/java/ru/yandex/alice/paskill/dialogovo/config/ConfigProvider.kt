package ru.yandex.alice.paskill.dialogovo.config

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import java.io.File
import java.nio.file.Files
import java.nio.file.Paths

@Configuration
internal open class ConfigProvider(private val objectMapper: ObjectMapper) {
    @Value("\${PG_MULTI_HOST}")
    private lateinit var pgDatabaseMultiHost: String

    @Value("\${PG_DATABASENAME}")
    private lateinit var pgDatabaseName: String

    @Value("\${PG_USER}")
    private lateinit var pgUser: String

    @Value("\${PG_PASSWORD}")
    private lateinit var pgPassword: String

    @Value("\${TVMTOOL_LOCAL_AUTHTOKEN:null}")
    private lateinit var yandexDeployTvmToken: String

    @Value("\${TVM_TOKEN:null}")
    private lateinit var tvmToken: String

    @Value("\${XIVA_TOKEN}")
    private lateinit var xivaToken: String

    @Value("\${DEVELOPER_TRUSTED_TOKEN:TRUSTED}")
    private lateinit var developerTrustedToken: String

    @Value("\${APP_METRICA_ENCRYPTION_SECRET:SECRET}")
    private lateinit var appMetricaEncryptionSecret: String

    @Value("\${YDB_ENDPOINT}")
    private lateinit var ydbEndpoint: String

    @Value("\${YDB_DATABASE}")
    private lateinit var ydbDatabase: String

    @Value("\${secretsConfigPath}")
    private val secretsConfigPath: String? = null

    private val logger = LogManager.getLogger()

    @Bean
    open fun secretsConfig(): SecretsConfig {
        if (pgDatabaseMultiHost.isNotBlank() &&
            pgDatabaseName.isNotBlank() &&
            pgUser.isNotBlank() &&
            pgPassword.isNotBlank()
        ) {
            return SecretsConfig(
                pgDatabaseMultiHost = pgDatabaseMultiHost,
                pgDatabaseName = pgDatabaseName,
                pgUser = pgUser,
                pgPassword = pgPassword,
                tvmToken = if ("null" != yandexDeployTvmToken) yandexDeployTvmToken else tvmToken,
                xivaToken = xivaToken,
                developerTrustedToken = developerTrustedToken,
                appMetricaEncryptionSecret = appMetricaEncryptionSecret,
                ydbEndpoint = ydbEndpoint,
                ydbDatabase = ydbDatabase,
                ytToken = ytToken(),
            )
        } else {
            val secretsConfigPath = File(secretsConfigPath!!)
            return objectMapper.readValue(secretsConfigPath)
        }
    }

    private fun ytToken(): String? {
        val token = System.getenv("YT_TOKEN") ?: try {

            val tokenPath = Paths.get(System.getProperty("user.home"), ".yt", "token")
            Files.newBufferedReader(tokenPath).use { reader -> reader.readLine() }
        } catch (e: Throwable) {
            logger.info("Yt token can't be read")
            null
        }
        return token
    }
}

