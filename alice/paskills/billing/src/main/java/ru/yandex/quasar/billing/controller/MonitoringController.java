package ru.yandex.quasar.billing.controller;

import java.io.IOException;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.common.io.CountingOutputStream;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.quasar.billing.monitoring.SolomonUtils;
import ru.yandex.quasar.billing.services.promo.PromoStatisticService;

@RestController
@RequestMapping(path = "/billing/monitoring/")
public class MonitoringController {
    private static final Logger log = LogManager.getLogger();

    private final PromoStatisticService promoStatisticService;
    private final MetricRegistry internalMetricRegistry;
    private final MetricRegistry promoMetricRegistry;

    public MonitoringController(
            PromoStatisticService promoStatisticService,
            @Qualifier("internalMetricRegistry") MetricRegistry internalMetricRegistry,
            @Qualifier("promoMetricRegistry") MetricRegistry promoMetricRegistry
    ) {
        this.promoStatisticService = promoStatisticService;
        this.internalMetricRegistry = internalMetricRegistry;
        this.promoMetricRegistry = promoMetricRegistry;
    }

    @GetMapping(value = "/solomon")
    public void getInternalSolomonSensors(HttpServletRequest request, HttpServletResponse response) throws IOException {
        dumpSolomonSensors(request, response, internalMetricRegistry);
    }

    @GetMapping(value = "/solomon/promo")
    public void getPromoSolomonSensors(HttpServletRequest request, HttpServletResponse response) throws IOException {
        promoStatisticService.writePromoStatisticsToSolomon();
        dumpSolomonSensors(request, response, promoMetricRegistry);
    }

    private void dumpSolomonSensors(HttpServletRequest request, HttpServletResponse response,
                                    MetricRegistry registry) throws IOException {
        var out = new CountingOutputStream(response.getOutputStream());

        try (MetricEncoder encoder = SolomonUtils.prepareEncoder(request, response, out)) {
            SolomonUtils.dump(registry, encoder);
        } catch (Exception e) {
            log.error("Error occurred while writing Solomon sensors", e);
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, e.getMessage());
        }

        response.setContentLengthLong(out.getCount());
    }
}
