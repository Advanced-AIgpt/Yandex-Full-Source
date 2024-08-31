package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import ru.yandex.alice.paskill.dialogovo.domain.Channel
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest
import java.util.UUID

internal class ShowFeedDaoTest : BaseDatabaseTest() {
    @Autowired
    private lateinit var showFeedDao: ShowFeedDao

    @Test
    fun testFindNothing() {
        Assertions.assertTrue(showFeedDao.findAllActiveShowFeeds().isEmpty())
    }

    @Test
    fun testFetch() {
        val logoUuid = UUID.randomUUID()
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
        insertSkillLogo(logoUuid, logoId, logoUrl)
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

        val showUuid = UUID.randomUUID()
        val showName = "test show"
        val showDescription = "test show description"

        insertShowFeed(
            feedId = showUuid,
            skillId = skillId,
            name = showName,
            description = showDescription
        )
        val expected = listOf(
            ShowInfoDB(
                id = showUuid,
                skillId = skillId,
                name = showName,
                nameTts = null,
                description = showDescription,
                onAir = true,
                type = ShowType.MORNING
            )
        )
        Assertions.assertEquals(expected, showFeedDao.findAllActiveShowFeeds())
    }
}
