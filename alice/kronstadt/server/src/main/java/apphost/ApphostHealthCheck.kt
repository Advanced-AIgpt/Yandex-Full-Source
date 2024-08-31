package ru.yandex.alice.kronstadt.server.apphost

import org.springframework.boot.availability.ApplicationAvailability
import org.springframework.boot.availability.LivenessState
import org.springframework.boot.availability.ReadinessState
import org.springframework.stereotype.Component
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheck
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheckResult

@Component
class ApphostHealthCheck(
    private val applicationAvailability: ApplicationAvailability,
) : AppHostHealthCheck {
    override fun check(): AppHostHealthCheckResult {
        return if (
            applicationAvailability.livenessState == LivenessState.CORRECT &&
            applicationAvailability.readinessState == ReadinessState.ACCEPTING_TRAFFIC
        ) {
            AppHostHealthCheckResult.healthy()
        } else {
            AppHostHealthCheckResult.unhealthy("Either liveness or readiness check failed")
        }
    }
}
