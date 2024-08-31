package ru.yandex.alice.paskill.dialogovo.config

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.context.annotation.Configuration
import org.springframework.core.env.Environment
import org.springframework.test.context.ActiveProfiles

@SpringBootTest(
    classes = [PropertiesPopulationTest.EmptyConfig::class],
    properties = ["spring.config.location=classpath:/config/dialogovo-config-prod.yaml"]
)
@ActiveProfiles("ut")
class PropertiesPopulationTest {

    @Autowired
    private lateinit var env: Environment

    @Value("\${show.yt.directory}")
    private lateinit var t: String

    @Test
    fun testPopulateProperties() {
        assertEquals("//home/paskills/skills-to-morning-show/prod", t)
    }

    @Configuration
    internal open class EmptyConfig
}
