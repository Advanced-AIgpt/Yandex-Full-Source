package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.mockito.ArgumentMatchers.anyString
import org.mockito.Mockito
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.boot.test.mock.mockito.MockBean
import ru.yandex.alice.kronstadt.test.AbstractIntegrationTest
import ru.yandex.alice.kronstadt.test.mockito.eq
import java.io.File
import java.time.Instant

@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@MockBean(ThereminSkillsDao::class)
class ThereminE2ETest : AbstractIntegrationTest(baseDir = "integration/theremin", scenarioMeta = THEREMIN) {

    @Autowired
    private lateinit var thereminSkillsDao: ThereminSkillsDao

    override fun mockServices(dir: File, uuidReplacement: String, now: Instant) {
        super.mockServices(dir, uuidReplacement, now)
        mockThereminInfo()
    }

    private fun mockThereminInfo() {
        Mockito.`when`(thereminSkillsDao.findThereminPrivateSkillsByUser(anyString()))
            .thenReturn(emptyList())

        val mockSkill = readContent("theremin_skill.json", ThereminSkillInfoDB::class.java)

        if (mockSkill != null) {
            Mockito.`when`(thereminSkillsDao.findThereminPrivateSkillsByUser(eq(mockSkill.userId)))
                .thenReturn(listOf(mockSkill))
        }
    }
}
