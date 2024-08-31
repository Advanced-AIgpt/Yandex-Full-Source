package ru.yandex.alice.paskill.dialogovo.controller.schedule

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty
import org.springframework.context.annotation.Lazy
import org.springframework.http.HttpStatus
import org.springframework.http.ResponseEntity
import org.springframework.web.bind.annotation.PathVariable
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.service.show.ShowService
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry

@RestController
@ConditionalOnProperty(name = ["scheduler.controllers.enable"], havingValue = "true")
class ShowController @Autowired constructor(
    @param:Lazy private val showService: ShowService,
    metricRegistry: MetricRegistry
) {
    private val logger = LogManager.getLogger()

    private val schedulerRunFailureCounter: Rate = metricRegistry.rate(
        "business_failure",
        Labels.of(
            "aspect", "show",
            "process", "update",
            "reason", "unhandled_alice_exception"
        )
    )

    @PostMapping(value = ["/show/{type}/update"])
    fun updateShows(@PathVariable(value = "type") type: String): ResponseEntity<Void> {
        return try {
            showService.updateUnpersonalizedShows(ShowType.valueOf(type))
            ResponseEntity.ok().build()
        } catch (ex: AliceHandledException) {
            logger.error("Scheduler got Alice handled error: " + ex.message, ex)
            schedulerRunFailureCounter.inc()
            ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build()
        } catch (ex: Exception) {
            logger.error("Scheduler got unexpected not AliceHandledException occurred", ex)
            throw ex
        }
    }
}
