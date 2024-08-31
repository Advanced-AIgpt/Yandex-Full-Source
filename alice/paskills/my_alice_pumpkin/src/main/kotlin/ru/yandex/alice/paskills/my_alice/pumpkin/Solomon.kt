package ru.yandex.alice.paskills.my_alice.pumpkin

import com.google.common.io.CountingOutputStream
import org.apache.logging.log4j.LogManager
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.stereotype.Component
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.monlib.metrics.JvmGc
import ru.yandex.monlib.metrics.JvmMemory
import ru.yandex.monlib.metrics.JvmRuntime
import ru.yandex.monlib.metrics.JvmThreads
import ru.yandex.monlib.metrics.encode.MetricEncoder
import ru.yandex.monlib.metrics.encode.MetricFormat
import ru.yandex.monlib.metrics.encode.json.MetricJsonEncoder
import ru.yandex.monlib.metrics.encode.spack.MetricSpackEncoder
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision
import ru.yandex.monlib.metrics.primitives.GaugeDouble
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.time.Instant
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse


@Configuration
open class SolomonConfiguration {

    @Bean
    open fun metricRegistry(): MetricRegistry {
        val registry = MetricRegistry.root()
        JvmGc.addMetrics(registry)
        JvmMemory.addMetrics(registry)
        JvmRuntime.addMetrics(registry)
        JvmThreads.addMetrics(registry)
        return registry
    }

}

@Component
class PumpkinAge(
    private val pumpkin: Pumpkin,
    metricRegistry: MetricRegistry
) {

    internal val age: GaugeDouble = metricRegistry.gaugeDouble("pumpkin.age.seconds")

    fun update() {
        val ageMs: Long = pumpkin.age().toMillis()
        age.set(ageMs.toDouble() / 1000)
    }
}

@RestController
@RequestMapping("/solomon")
class SolomonController(
    private val pumpkinAge: PumpkinAge,
    private val metricRegistry: MetricRegistry
) {

    private val logger = LogManager.getLogger()

    private val HTTP_HEADER_ACCEPT = "Accept"

    @RequestMapping("")
    fun solomon(request: HttpServletRequest, response: HttpServletResponse) {
        pumpkinAge.update()
        val acceptHeader: String? = request.getHeader(HTTP_HEADER_ACCEPT)
        logger.debug("Handling solomon request")
        val outputStream = CountingOutputStream(response.outputStream)
        val encoder: MetricEncoder
        val contentType: String
        if ("application/json".equals(acceptHeader)) {
            logger.debug("Using JSON encoder")
            encoder = MetricJsonEncoder(outputStream)
            contentType = MetricFormat.JSON.contentType()
        } else {
            logger.debug("Using spack encoder")
            encoder = MetricSpackEncoder(TimePrecision.SECONDS, CompressionAlg.LZ4, outputStream);
            contentType = MetricFormat.SPACK.contentType()
        }
        response.contentType = contentType;
        encoder.use { metricRegistry.supply(0, encoder) }
    }

}
