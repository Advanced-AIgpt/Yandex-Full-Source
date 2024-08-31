package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest
import java.util.UUID

class UserAgreementDaoTest : BaseDatabaseTest() {

    private val userId = "123"

    @Autowired
    private lateinit var userAgreementDao: UserAgreementDao

    @BeforeEach
    fun setUp() {
        insertUser(userId, "username")
    }

    @Test
    fun testFindNothing() {
        assertEquals(0, userAgreementDao.findAllPublishedUserAgreements().size)
    }

    private fun insertSkill(): UUID {
        val skillId = UUID.randomUUID()
        val logoId = UUID.randomUUID()
        insertSkillLogo(
            skillId,
            logoId,
            "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig"
        )
        insertSkill(
            id = skillId,
            salt = UUID.randomUUID(),
            userId = userId,
            name = "skill name",
            slug = "skill-slug",
            logoId = logoId,
        )
        return skillId
    }

    private fun validateUserAgreementDb(
        id: UUID,
        skillId: UUID,
        name: String,
        url: String,
        order: Int,
        userAgreement: UserAgreementDb,
    ) {
        assertEquals(id, userAgreement.id)
        assertEquals(skillId, userAgreement.skillId)
        assertEquals(name, userAgreement.name)
        assertEquals(url, userAgreement.url)
        assertEquals(order, userAgreement.order)
    }

    @Test
    fun testDraftUserAgreement() {
        val skillId = insertSkill()
        val id = UUID.randomUUID()
        val name = "name"
        val url = "https://yandex.ru"
        val order = 0
        insertDraftUserAgreement(
            id,
            skillId,
            order,
            name,
            url,
        )
        val draftUserAgreements = userAgreementDao.findDraftUserAgreements(skillId.toString())
        assertEquals(1, draftUserAgreements.size)
        val draftUserAgreement = draftUserAgreements[0]
        validateUserAgreementDb(id, skillId, name, url, order, draftUserAgreement)
    }

    @Test
    fun testDraftUserAgreementFromTwoSkills() {
        val firstSkillId = insertSkill()
        val firstUserAgreementId = UUID.randomUUID()
        val name = "name 1"
        val url = "https://yandex.ru"
        val order = 0
        insertDraftUserAgreement(
            firstUserAgreementId,
            firstSkillId,
            order,
            name,
            url,
        )
        val secondSkillId = insertSkill()
        val secondUserAgreementId = UUID.randomUUID()
        insertDraftUserAgreement(
            secondUserAgreementId,
            secondSkillId,
            order,
            name + "2",
            url + "/2",
        )

        val draftUserAgreements = userAgreementDao.findDraftUserAgreements(firstSkillId.toString())
        assertEquals(1, draftUserAgreements.size)
        val draftUserAgreement = draftUserAgreements[0]
        validateUserAgreementDb(firstUserAgreementId, firstSkillId, name, url, order, draftUserAgreement)
    }

    @Test
    fun testPublishedUserAgreements() {
        val url = "https://yandex.ru"
        val firstSkillId = insertSkill()
        val firstUserAgrement = UserAgreementDb(
            UUID.randomUUID(),
            firstSkillId,
            0,
            "name 1",
            "https://yandex.ru"
        )
        insertPublishedUserAgreement(firstUserAgrement)
        val secondSkillId = insertSkill()
        val secondUserAgreement = UserAgreementDb(
            UUID.randomUUID(),
            secondSkillId,
            0,
            "name 2",
            url + "/2"
        )
        insertPublishedUserAgreement(secondUserAgreement)

        val userAgreements = userAgreementDao.findAllPublishedUserAgreements()
        assertEquals(
            userAgreements.toSet(),
            setOf(firstUserAgrement, secondUserAgreement)
        )
    }
}
