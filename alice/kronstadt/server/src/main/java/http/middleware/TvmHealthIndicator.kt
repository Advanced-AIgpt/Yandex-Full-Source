package ru.yandex.alice.kronstadt.server.http.middleware

import org.apache.logging.log4j.LogManager
import org.springframework.boot.actuate.health.Health
import org.springframework.boot.actuate.health.HealthIndicator
import org.springframework.stereotype.Component
import ru.yandex.passport.tvmauth.ClientStatus
import ru.yandex.passport.tvmauth.TvmClient

// indicator name is `tvm`
@Component
internal open class TvmHealthIndicator(private val tvmClient: TvmClient) : HealthIndicator {

    private val logger = LogManager.getLogger()

    override fun health(): Health {
        val status = tvmClient.status
        if (status.code == ClientStatus.Code.OK) {
            return Health.up().build()
        } else {
            logger.warn("TVM Client is down!!! ${status.lastError}")
            return Health.outOfService().withDetail("last_error", status.lastError).build()
        }
    }
}
