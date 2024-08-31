package ru.yandex.alice.paskill.dialogovo.test

import com.fasterxml.jackson.core.JsonProcessingException
import com.fasterxml.jackson.databind.ObjectMapper
import org.junit.jupiter.api.extension.ExtendWith
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.autoconfigure.jackson.JacksonAutoConfiguration
import org.springframework.boot.test.context.TestConfiguration
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Primary
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.jdbc.core.JdbcTemplate
import org.springframework.jdbc.core.namedparam.MapSqlParameterSource
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate
import org.springframework.jdbc.datasource.DataSourceTransactionManager
import org.springframework.jdbc.datasource.DelegatingDataSource
import org.springframework.test.context.TestPropertySource
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig
import org.springframework.transaction.PlatformTransactionManager
import org.springframework.transaction.annotation.EnableTransactionManagement
import ru.yandex.alice.paskill.dialogovo.EmbeddedPostgresExecutionListener
import ru.yandex.alice.paskill.dialogovo.EmbeddedPostgresExtension
import ru.yandex.alice.paskill.dialogovo.config.ConfigProvider
import ru.yandex.alice.paskill.dialogovo.config.DialogovoJdbcConfiguration
import ru.yandex.alice.paskill.dialogovo.config.DialogovoRepositoryConfiguration
import ru.yandex.alice.paskill.dialogovo.domain.Channel
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB
import ru.yandex.alice.paskill.dialogovo.providers.skill.UserAgreementDb
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.PaskillsJdbcConfiguration
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminConfiguration
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminPgConverters
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminRepositoryConfig
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest.TransactionManagerConfig
import java.nio.charset.StandardCharsets
import java.sql.Connection
import java.sql.PreparedStatement
import java.sql.SQLException
import java.sql.Types
import java.util.UUID
import javax.sql.DataSource

@ExtendWith(EmbeddedPostgresExtension::class)
@SpringJUnitConfig(
    classes = [
        DialogovoJdbcConfiguration::class,
        JacksonAutoConfiguration::class,
        TransactionManagerConfig::class,
        PaskillsJdbcConfiguration::class,
        ThereminPgConverters::class,
        ThereminConfiguration::class,
        ConfigProvider::class,
        ThereminRepositoryConfig::class,
        DialogovoRepositoryConfiguration::class,
    ],
)
@TestPropertySource("classpath:/application.properties")
abstract class BaseDatabaseTest {

    @Autowired
    protected lateinit var jdbcTemplate: JdbcTemplate

    @Autowired
    protected lateinit var objectMapper: ObjectMapper

    @Autowired
    protected lateinit var namedParameterJdbcTemplate: NamedParameterJdbcTemplate

    private val resolver = PathMatchingResourcePatternResolver()

    @Throws(JsonProcessingException::class, SQLException::class)
    protected fun insertSkill(
        id: UUID,
        salt: UUID,
        persistentUserIdSalt: UUID = salt,
        userId: String,
        name: String,
        slug: String,
        onAir: Boolean = true,
        backendSettings: SkillInfoDB.BackendSettings = SkillInfoDB.BackendSettings("http://localhost/testskill", null),
        publishingSettings: SkillInfoDB.PublishingSettings = EMPTY_PUBLISHING_SETTINGS,
        logoId: UUID,
        featureFlags: List<String> = listOf(),
        channel: Channel = Channel.ALICE_SKILL,
        developerType: DeveloperType = DeveloperType.External,
        hideInStore: Boolean = false,
        isRecommended: Boolean? = true,
        automaticIsRecommended: Boolean = true,
        inflectedActivationPhrases: List<String> = listOf(),
        rsyPlatformId: String? = null,
        score: Double = 1.0,
        requiredInterfaces: List<String> = listOf(),
        skillAccess: String? = "public",
        exposeInternalFlags: Boolean = false,
        tags: List<String>? = listOf(),
        editorDescription: String? = null,
    ) {
        val sql = getStringResource("skills_dao/insert_skill.sql")
        val backendSettingsString = objectMapper.writeValueAsString(backendSettings)
        val publishingSettingsString = objectMapper.writeValueAsString(publishingSettings)
        val connection = jdbcTemplate.dataSource!!.connection
        jdbcTemplate.update(sql) { ps: PreparedStatement ->
            ps.setObject(1, id)
            ps.setObject(2, salt)
            ps.setObject(3, persistentUserIdSalt)
            ps.setString(4, userId)
            ps.setString(5, name)
            ps.setString(6, slug)
            ps.setBoolean(7, onAir)
            ps.setString(8, backendSettingsString)
            ps.setString(9, publishingSettingsString)
            ps.setObject(10, logoId)
            ps.setArray(11, connection.createArrayOf("text", featureFlags.toTypedArray()))
            ps.setString(12, channel.value)
            ps.setString(13, developerType.value)
            ps.setBoolean(14, hideInStore)
            if (isRecommended == null) {
                ps.setNull(15, Types.BOOLEAN)
            } else {
                ps.setBoolean(15, isRecommended)
            }
            ps.setBoolean(16, automaticIsRecommended)
            ps.setArray(17, connection.createArrayOf("text", inflectedActivationPhrases.toTypedArray()))
            ps.setString(18, rsyPlatformId)
            ps.setDouble(19, score)
            ps.setArray(20, connection.createArrayOf("text", requiredInterfaces.toTypedArray()))
            ps.setString(21, skillAccess)
            ps.setBoolean(22, exposeInternalFlags)
            if (tags == null) {
                ps.setNull(23, Types.ARRAY)
            } else {
                ps.setArray(23, connection.createArrayOf("varchar", tags.toTypedArray()))
            }
            ps.setString(24, editorDescription)
        }
    }

    protected fun insertUser(id: String, username: String) {
        jdbcTemplate.update(getStringResource("skills_dao/insert_user.sql"), id, username)
    }

    protected fun getStringResource(location: String): String {
        return resolver.getResource(location).file.readText(StandardCharsets.UTF_8)
        /*return try {
            Files.asCharSource(, ).read()
        } catch (e: IOException) {
            throw RuntimeException(e)
        }*/
    }

    protected fun insertUserShare(skillId: UUID, userId: String) {
        jdbcTemplate.update(
            getStringResource("skills_dao/insert_share.sql"),
            UUID.randomUUID(),
            skillId,
            userId
        )
    }

    protected fun insertIntent(skillId: UUID, formName: String, humanReadableName: String, isActivation: Boolean) {
        jdbcTemplate.update(
            getStringResource("skills_dao/insert_intent.sql"),
            UUID.randomUUID(),
            skillId,
            formName,
            humanReadableName,
            isActivation
        )
    }

    protected fun insertShowFeed(
        feedId: UUID,
        skillId: UUID,
        name: String,
        description: String
    ) {
        jdbcTemplate.update(
            getStringResource("skills_dao/insert_show_feed.sql"),
            feedId,
            skillId,
            name,
            description
        )
    }

    protected fun insertSkillLogo(uuid: UUID, logoId: UUID, s: String) {
        jdbcTemplate.update(
            getStringResource("skills_dao/insert_image.sql"),
            logoId, uuid, s,
            "skillSettings"
        )
    }

    protected fun insertSkillCrypto(uuid: UUID) {
        this.namedParameterJdbcTemplate.update(
            """
            insert into "skillsCrypto" (
                "skillId"
                , "publicKey"
                , "privateKey"
            ) values (
                :uuid
                , :publicKey
                , :privateKey
            )
            """.trimIndent(),
            MapSqlParameterSource("uuid", uuid)
                .addValue("publicKey", PUBLIC_KEY)
                .addValue("privateKey", PRIVATE_KEY)
        )
    }

    protected fun insertUserAgreement(
        resourceName: String,
        id: UUID,
        skillId: UUID,
        order: Int,
        name: String,
        url: String,
    ) {
        jdbcTemplate.update(
            getStringResource(resourceName),
            id,
            skillId,
            order,
            name,
            url,
        )
    }

    protected fun insertDraftUserAgreement(
        id: UUID,
        skillId: UUID,
        order: Int,
        name: String,
        url: String,
    ) = insertUserAgreement(
        "skills_dao/insert_draft_user_agreement.sql",
        id,
        skillId,
        order,
        name,
        url,
    )

    protected fun insertPublishedUserAgreement(ua: UserAgreementDb) = insertUserAgreement(
        "skills_dao/insert_published_user_agreement.sql",
        ua.id,
        ua.skillId,
        ua.order,
        ua.name,
        ua.url,
    )

    @EnableTransactionManagement
    @TestConfiguration
    internal open class TransactionManagerConfig {

        @Bean("dataSource", "testDataSource")
        @Primary
        open fun testDataSource(): DataSource {
            return object : DelegatingDataSource(EmbeddedPostgresExecutionListener.createDataSource()) {
                private val ds = EmbeddedPostgresExecutionListener.createDataSource()

                @Throws(SQLException::class)
                override fun getConnection(): Connection {
                    return ds.connection
                }
            }
        }

        @Bean
        open fun namedParameterJdbcOperation() =
            NamedParameterJdbcTemplate(testDataSource())

        @Bean
        open fun transactionManager(): PlatformTransactionManager {
            return DataSourceTransactionManager(testDataSource())
        }

        @Bean
        open fun jdbcTemplate(): JdbcTemplate {
            return JdbcTemplate(testDataSource())
        }
    }

    companion object {
        const val PUBLIC_KEY = "public_key_123"
        const val PRIVATE_KEY = "private_key_321"

        @JvmField
        val EMPTY_PUBLISHING_SETTINGS = SkillInfoDB.PublishingSettings(
            description = null,
            category = null,
            examples = null,
            developerName = null,
            brandVerificationWebsite = null,
            explicitContent = null,
            structuredExamples = listOf()
        )
    }
}
