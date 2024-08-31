package ru.yandex.alice.divktrenderer.controller

import com.google.common.io.CountingOutputStream
import org.apache.logging.log4j.LogManager
import org.springframework.http.ResponseEntity
import org.springframework.web.bind.annotation.GetMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.divktrenderer.solomon.SolomonUtils
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.io.IOException
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@RestController
internal class MonitoringController(private val registry: MetricRegistry) {
    @GetMapping("/solomon")
    @Throws(IOException::class)
    fun getSolomonMetrics(request: HttpServletRequest, response: HttpServletResponse) {
        val out = CountingOutputStream(response.outputStream)
        try {
            SolomonUtils.prepareEncoder(request, response, out).use { encoder -> SolomonUtils.dump(registry, encoder) }
        } catch (e: Exception) {
            logger.error("Error occurred while writing Solomon sensors", e)
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, e.message)
        }
        response.setContentLengthLong(out.count)
    }

    @GetMapping("/healthcheck")
    fun healthcheck(): ResponseEntity<String> = ResponseEntity.ok("OK")

    companion object {
        private val logger = LogManager.getLogger()
    }
}
