package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.apache.logging.log4j.LogManager
import org.springframework.boot.actuate.health.Health
import org.springframework.boot.actuate.health.HealthIndicator
import org.springframework.stereotype.Component

// indicator name is `theremin`
@Component
internal open class ThereminHealthIndicator(private val thereminService: PlayScene) : HealthIndicator {

    private val logger = LogManager.getLogger()

    override fun health(): Health = if (thereminService.isReady) {
        Health.up().build()
    } else {
        logger.warn("theremin health indicator is DOWN")
        Health.down().build()
    }
}
