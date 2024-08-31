package ru.yandex.alice.paskill.dialogovo.controller

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.web.bind.annotation.GetMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.io.IOException
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@RestController
internal class MainController(
    @param:Qualifier("externalMetricRegistry")
    private val externalMetricRegistry: MetricRegistry
) {

    @GetMapping(value = ["/solomon/external"])
    @Throws(IOException::class)
    fun getExternalSensors(request: HttpServletRequest, response: HttpServletResponse) {
        SolomonUtils.dumpSolomonSensorsToResponse(request, response, externalMetricRegistry)
    }
}
