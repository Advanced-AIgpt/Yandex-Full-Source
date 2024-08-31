package ru.yandex.alice.paskill.dialogovo.providers.skill

import com.fasterxml.jackson.core.JsonProcessingException
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.paskill.dialogovo.domain.Channel
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB.PublishingSettings
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB.StructuredExample
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillInfoDB
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillInfoDB.Sound
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillInfoDB.SoundSet
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillsDao
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest
import java.sql.SQLException
import java.sql.Timestamp
import java.time.Instant
import java.util.Locale
import java.util.UUID
import java.util.stream.Collectors
import java.util.stream.Stream

internal class SkillsDaoTest : BaseDatabaseTest() {
    @Autowired
    private lateinit var skillsDao: SkillsDao

    @Autowired
    private lateinit var thereminSkillsDao: ThereminSkillsDao

    @Test
    fun testFindNothing() {
        Assertions.assertFalse(skillsDao.findAliceSkillById(UUID.randomUUID()).isPresent)
    }

    @Test
    @Throws(JsonProcessingException::class, SQLException::class)
    fun testSaving() {
        val uuid = UUID.randomUUID()
        val logoId = UUID.randomUUID()
        val salt = UUID.randomUUID()
        val userId = "123"
        insertUser(userId, "username")
        insertSkillLogo(uuid, logoId, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        val skillFeatureFlags = listOf("test_flag", "test_flag2")
        val inflectedActivationPhrases = listOf("ботинок", "ботинка")
        insertSkill(
            id = uuid,
            salt = salt,
            persistentUserIdSalt = salt,
            userId = userId,
            name = "test skill",
            slug = "slug",
            onAir = true,
            backendSettings = SkillInfoDB.BackendSettings("http://localhost/testskill", null),
            publishingSettings = EMPTY_PUBLISHING_SETTINGS,
            logoId = logoId,
            featureFlags = skillFeatureFlags,
            channel = Channel.ALICE_SKILL,
            developerType = DeveloperType.Yandex,
            hideInStore = false,
            isRecommended = null,
            automaticIsRecommended = true,
            inflectedActivationPhrases = inflectedActivationPhrases,
            rsyPlatformId = "rsyId",
            score = 1.0,
            requiredInterfaces = emptyList(),
            skillAccess = null,
            exposeInternalFlags = true,
            tags = listOf("games", "onboarding"),
            editorDescription = null
        )
        insertSkillCrypto(uuid)
        val actual = skillsDao.findAliceSkillById(uuid).orElseThrow {
            RuntimeException("skill not found")
        }
        assertEquals("test skill", actual.name)
        Assertions.assertNull(actual.isRecommended)
        assertEquals(skillFeatureFlags, actual.featureFlags)
        assertEquals(actual.userFeatureFlags, mapOf("abc" to 123))
        assertEquals(listOf<Any>(), actual.sharedAccess)
        val allSkills = skillsDao.findAllActiveAliceSkill()
        assertEquals(listOf(actual), allSkills)
        assertEquals(actual.publicKey, PUBLIC_KEY)
        assertEquals(actual.privateKey, PRIVATE_KEY)
        Assertions.assertTrue(actual.exposeInternalFlags)
        val actual2 = skillsDao.findByActivationPhrases(
            arrayOf(
                "ботинок", "ящик",
                "красный шар"
            )
        )
        assertEquals(
            listOf(SkillIdPhrasesEntity(uuid, listOf("ботинок", "ботинка"))),
            actual2
        )
    }

    @Test
    fun testQueryPhrases() {
        assertEquals(listOf<Any>(), skillsDao.findByActivationPhrases(arrayOf("ботинок")))
    }

    @Test
    @Throws(JsonProcessingException::class, SQLException::class)
    fun testSavingWithSurfaces() {
        val uuid = UUID.randomUUID()
        val logoId = UUID.randomUUID()
        val salt = UUID.randomUUID()
        val userId = "123"
        insertUser(userId, "username")
        insertSkillLogo(uuid, logoId, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        insertSkill(
            uuid,
            salt,
            salt,
            userId,
            "test skill",
            "slug",
            true,
            SkillInfoDB.BackendSettings("http://localhost/testskill", null),
            EMPTY_PUBLISHING_SETTINGS,
            logoId,
            listOf("test_flag", "test_flag2"),
            Channel.ALICE_SKILL,
            DeveloperType.Yandex,
            false,
            true,
            true,
            emptyList(),
            null,
            1.0,
            listOf("screen", "browser"),
            null,
            false,
            null,
            null
        )
        val skill: SkillInfoDB = SkillInfoDB(
            id = UUID.randomUUID(),
            name = "test skill",
            userId = "123",
            onAir = true,
            isRecommended = true,
            hideInStore = false,
            salt = "salt",
            backendSettings = SkillInfoDB.BackendSettings("http://localhost/testskill", null),
            requiredInterfaces = listOf("screen", "browser"),

            logoUrl = "",
            publishingSettings = PublishingSettings(
                description = "",
                category = "",
                examples = emptyList(),
                developerName = "",
                brandVerificationWebsite = "",
                explicitContent = false,
                structuredExamples = emptyList()
            ),
            slug = "",
            look = SkillInfo.Look.EXTERNAL.code,
            voice = Voice.OKSANA.code,
            featureFlags = listOf(),
            automaticIsRecommended = true,
            developerType = "External",
            draft = false,
            exposeInternalFlags = false,
            persistentUserIdSalt = "salt",
            nameTts = null,
            skillAccess = "public",
            useNlu = true,
            useZora = true,
            monitoringType = SkillInfo.MonitoringType.NONMONITORED.code,
        )
        val actual = skillsDao.findAliceSkillById(uuid).orElseThrow {
            RuntimeException(
                "skill not " +
                    "found"
            )
        }
        assertEquals("test skill", actual.name)
        assertEquals(true, skill.isRecommended)
        assertEquals(listOf("test_flag", "test_flag2"), actual.featureFlags)
        assertEquals(actual.userFeatureFlags, mapOf("abc" to 123))
        assertEquals(listOf("screen", "browser"), actual.requiredInterfaces)
        assertEquals(listOf<Any>(), actual.sharedAccess)
    }

    @Test
    @Throws(JsonProcessingException::class, SQLException::class)
    fun testSkillEquals() {
        val uuid = UUID.randomUUID()
        val logoId = UUID.randomUUID()
        val salt = UUID.randomUUID()
        val userId = "123"
        val userId2 = "1234"
        val backendUrl = "http://localhost/testskill"
        val slug = "slug"
        val name = "test skill"
        val skillId = UUID.randomUUID()
        insertUser(userId, "username")
        insertUser(userId2, "username")
        val logoUrl = "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig"
        insertSkillLogo(uuid, logoId, logoUrl)
        insertSkill(
            skillId,
            salt,
            salt,
            userId,
            name,
            slug,
            true,
            SkillInfoDB.BackendSettings(backendUrl, null),
            EMPTY_PUBLISHING_SETTINGS,
            logoId,
            listOf("test_flag", "test_flag2"),
            Channel.ALICE_SKILL,
            DeveloperType.External,
            true,
            true,
            true,
            emptyList(),
            null,
            1.0,
            listOf("screen", "browser"),
            SkillInfo.SkillAccess.PRIVATE.value,
            false,
            listOf("games", "onboarding"),
            "Описание навыка"
        )
        insertUserShare(skillId, userId2)
        val skill = SkillInfoDB(
            id = skillId,
            name = name,
            userId = "123",
            onAir = true,
            isRecommended = true,
            automaticIsRecommended = true,
            slug = slug,
            hideInStore = true,
            salt = salt.toString(),
            persistentUserIdSalt = salt.toString(),
            voice = Voice.OKSANA.code,
            useZora = true,
            backendSettings = SkillInfoDB.BackendSettings(backendUrl, null),
            channel = "aliceSkill",
            look = SkillInfo.Look.EXTERNAL.code,
            logoUrl = logoUrl,
            useNlu = false,
            developerType = DeveloperType.External.name.lowercase(Locale.ROOT),
            _userFeatureFlags = SkillInfoDB.UserFeatures(mapOf("abc" to 123)),
            featureFlags = listOf("test_flag", "test_flag2"),
            publishingSettings = PublishingSettings(
                description = null,
                category = null,
                examples = null,
                developerName = null,
                brandVerificationWebsite = null,
                explicitContent = null,
                structuredExamples = listOf()
            ),
            requiredInterfaces = listOf("screen", "browser"),
            useStateStorage = false,
            score = 1.0,
            skillAccess = "private",
            sharedAccess = listOf(userId2),
            tags = listOf("games", "onboarding"),
            nameTts = null,
            draft = false,
            exposeInternalFlags = false,
            editorDescription = "Описание навыка",
            monitoringType = SkillInfo.MonitoringType.NONMONITORED.code,
        )
        val actual = skillsDao.findAliceSkillById(skillId).orElseThrow {
            RuntimeException(
                "skill not " +
                    "found"
            )
        }
        assertEquals(skill, actual)
        val allSkills = skillsDao.findAllActiveAliceSkill()
        assertEquals(listOf(actual), allSkills)
    }

    @Test
    @Throws(JsonProcessingException::class)
    fun testParsePublishingSettings() {
        val expexted = PublishingSettings(
            """
                  Долгожданное обновление игры!

                  Агент, 4 миссии ждут вас прямо сейчас! Используйте свои навыки и чутьё, чтобы полностью выполнить назначенные миссии.
            """.trimIndent(),
            "games_trivia_accessories",
            null,
            "secret agent.",
            "",
            false,
            listOf(
                StructuredExample("запусти навык", "секретный агент", ""),
                StructuredExample("поиграем в", "секретного агента", ""),
                StructuredExample("давай сыграем в", "секретного агента", "")
            )
        )
        val actual = objectMapper.readValue(
            getStringResource("skills_dao/test_publishing_settings.json"), PublishingSettings::class.java
        )
        assertEquals(expexted, actual)
    }

    @Test
    @Throws(JsonProcessingException::class)
    fun testSavingTheremin() {
        val uuid = UUID.randomUUID()
        val logoId = UUID.randomUUID()
        val salt = UUID.randomUUID()
        val userId = "123"
        insertUser(userId, "username")
        insertSkillLogo(uuid, logoId, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        val backendSettings = ThereminSkillInfoDB.BackendSettings(
            SoundSet(
                Stream.of(
                    Sound("id", "path"),
                    null // it may contain null
                ).collect(Collectors.toList()),
                ThereminSkillInfoDB.Settings(true, true, true)
            )
        )
        jdbcTemplate.update(
            "insert into skills (id, \"createdAt\", \"updatedAt\", salt, \"userId\", name, \"onAir\"," +
                " \"backendSettings\", \"logoId\", \"featureFlags\", channel, \"developerType\", " +
                "\"hideInStore\") " +
                "values (?, ?, ?, ?, ?, ?, ?, (?)::json, ?, '{test_flag, test_flag2}',?::enum_skills_channel," +
                " ?, ?)",
            uuid,
            Timestamp.from(Instant.now()),
            Timestamp.from(Instant.now()),
            salt, userId, "test skill", true,
            objectMapper.writeValueAsString(backendSettings),
            logoId,
            "thereminvox",
            "external",
            true // List.of("test_flag")
        )
        val skill = ThereminSkillInfoDB(
            id = uuid,
            name = "test skill",
            onAir = true,
            userId = userId,
            backendSettings = backendSettings,
            developerType = "external",
            hideInStore = true,
        )
        val actual = this.thereminSkillsDao.findThereminPrivateSkillsByUser(userId)
        assertEquals(listOf(skill), actual)
    }

    @Test
    @Throws(JsonProcessingException::class)
    fun testSavingThereminPublic() {
        val uuid = UUID.randomUUID()
        val logoId = UUID.randomUUID()
        val salt = UUID.randomUUID()
        val userId = "123"
        insertUser(userId, "username")
        insertSkillLogo(uuid, logoId, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        val backendSettings = ThereminSkillInfoDB.BackendSettings(
            SoundSet(
                Stream.of(
                    Sound("id", "path"),
                    null // it may contain null
                ).collect(Collectors.toList()),
                ThereminSkillInfoDB.Settings(true, true, true)
            )
        )
        jdbcTemplate.update(
            "insert into skills (id, \"createdAt\", \"updatedAt\", salt, \"userId\", name, \"onAir\"," +
                " \"backendSettings\", \"logoId\", \"featureFlags\", channel, \"developerType\", " +
                "\"hideInStore\", \"inflectedActivationPhrases\") " +
                "values (?, ?, ?, ?, ?, ?, ?, (?)::json, ?, '{test_flag, test_flag2}',?::enum_skills_channel," +
                " ?, ?,'{оператор,операторы,оператора,операторов,оператору,операторам,оператором,операторами," +
                "операторе,операторах}')",
            uuid,
            Timestamp.from(Instant.now()),
            Timestamp.from(Instant.now()),
            salt, userId, "test skill", true,
            objectMapper.writeValueAsString(backendSettings),
            logoId,
            "thereminvox",
            "external",
            false // List.of("test_flag")
        )
        val skill = ThereminSkillInfoDB(
            id = uuid,
            name = "test skill",
            onAir = true,
            userId = userId,
            backendSettings = backendSettings,
            developerType = "external",
            hideInStore = false,
            inflectedActivationPhrases = listOf(
                "оператор", "операторы", "оператора", "операторов", "оператору", "операторам", "оператором",
                "операторами", "операторе", "операторах"
            ),
        )
        val actual = thereminSkillsDao.findThereminAllPublicSkills()
        assertEquals(listOf(skill), actual)
    }

    @Test
    fun testGetAllActivationIntents() {

        val salt = UUID.randomUUID()
        val userId = "123"
        insertUser(userId, "username")

        val skillId1 = UUID.randomUUID()
        val logoId1 = UUID.randomUUID()
        insertSkillLogo(skillId1, logoId1, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        insertSkill(
            id = skillId1,
            salt = salt,
            userId = userId,
            name = "test skill",
            slug = "slug",
            logoId = logoId1,
        )
        val skillId2 = UUID.randomUUID()
        val logoId2 = UUID.randomUUID()
        insertSkillLogo(skillId2, logoId2, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        insertSkill(
            id = skillId2,
            salt = salt,
            userId = userId,
            name = "test skill",
            slug = "slug",
            logoId = logoId2,
        )
        val skillId3 = UUID.randomUUID()
        val logoId3 = UUID.randomUUID()
        insertSkillLogo(skillId3, logoId3, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig")
        insertSkill(
            id = skillId3,
            salt = salt,
            userId = userId,
            name = "test skill",
            slug = "slug",
            logoId = logoId3,
        )
        insertIntent(skillId1, "uno", "один", false)
        insertIntent(skillId2, "uno", "один", false)
        insertIntent(skillId1, "run", "запускай", true)
        insertIntent(skillId1, "gogogo", "поехали", true)
        insertIntent(skillId3, "rere", "давай", true)
        val expected: List<SkillActivationIntentsEntity> = ArrayList(
            listOf(
                SkillActivationIntentsEntity(skillId1, listOf("run", "gogogo")),
                SkillActivationIntentsEntity(skillId3, listOf("rere"))
            )
        ).sortedBy { it.id }
        val got = skillsDao.findAllActivationIntents().sortedBy { it.id }
        assertEquals(expected, got)
    }
}
