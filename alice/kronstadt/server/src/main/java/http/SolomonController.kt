package ru.yandex.alice.kronstadt.server.http

import org.springframework.web.bind.annotation.GetMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.io.IOException
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@RestController
class SolomonController internal constructor(
    private val internalMetricRegistry: MetricRegistry
) {
    @GetMapping(value = ["/solomon"])
    @Throws(IOException::class)
    fun getInternalSensors(request: HttpServletRequest, response: HttpServletResponse) {
        SolomonUtils.dumpSolomonSensorsToResponse(request, response, internalMetricRegistry)
    }
}
