package ru.yandex.alice.paskill.dialogovo.midleware

import org.apache.logging.log4j.LogManager
import org.springframework.boot.actuate.health.Health
import org.springframework.boot.actuate.health.HealthIndicator
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.scenarios.alice4business.Alice4BusinessService
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider
import java.sql.SQLException
import javax.sql.DataSource

// indicator name is `skillProvider`
@Component
internal open class SkillProviderHealthIndicator(private val skillProvider: SkillProvider) : HealthIndicator {

    private val logger = LogManager.getLogger()

    override fun health(): Health = if (skillProvider.isReady()) {
        Health.up().build()
    } else {
        logger.warn("skillProvider health indicator is DOWN")
        Health.down().build()
    }
}

// indicator name is `newsSkillProvider`
@Component
internal open class NewsSkillProviderHealthIndicator(private val skillProvider: NewsSkillProvider) : HealthIndicator {

    private val logger = LogManager.getLogger()

    override fun health(): Health = if (skillProvider.isReady) {
        Health.up().build()
    } else {
        logger.warn("newsSkillProvider health indicator is DOWN")
        Health.down().build()
    }
}

// indicator name is `pg`
@Component
internal open class PgHealthIndicator(private val dataSources: List<DataSource>) : HealthIndicator {

    private val logger = LogManager.getLogger()

    override fun health(): Health {
        return try {
            dataSources.forEach { it.connection.use { /*nothing*/ } }
            Health.up().build()
        } catch (e: SQLException) {
            logger.warn("pg health indicator is DOWN")
            Health.down().withException(e).build()
        }
    }
}

// indicator name is `a4b`
@Component("a4bHealthIndicator")
internal open class Alice4BusinessHealthIndicator(private val alice4BusinessService: Alice4BusinessService) :
    HealthIndicator {

    private val logger = LogManager.getLogger()

    override fun health(): Health = if (alice4BusinessService.isReady) {
        Health.up().build()
    } else {
        logger.warn("a4b health indicator is DOWN")
        Health.down().build()
    }
}
