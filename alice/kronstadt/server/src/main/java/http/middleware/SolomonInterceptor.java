package ru.yandex.alice.kronstadt.server.http.middleware;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.function.Supplier;
import java.util.regex.Pattern;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;
import org.springframework.web.method.HandlerMethod;
import org.springframework.web.servlet.HandlerInterceptor;

import ru.yandex.alice.paskills.common.solomon.utils.RequestSensors;
import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.springframework.util.StringUtils.trimLeadingCharacter;
import static org.springframework.util.StringUtils.trimTrailingCharacter;

@Component
public class SolomonInterceptor implements HandlerInterceptor {
    public static final String METRICS_START_TIME_ATTRIBUTE = "metricsStartTimeAttribute";
    private static final Logger logger = LogManager.getLogger();
    private final MetricRegistry metricRegistry;
    private final Rate allRequestRate;
    private final ConcurrentMap<String, RequestSensors> requestCounterCache = new ConcurrentHashMap<>();
    private final Supplier<HistogramCollector> histogramCollectorSupplier = () ->
            Histograms.explicit(10, 30, 60, 90, 120, 150, 200, 250, 260, 280, 290, 300, 310, 320, 400, 500, 1000,
                    1500, 2000, 2500, 2800, 2900, 3000, 3100, 3200, 3300, 3500, 4000);

    private final Pattern signalNamePattern = Pattern.compile("[/\\-.]");

    public SolomonInterceptor(MetricRegistry metricRegistry) {
        this.metricRegistry = metricRegistry;
        this.allRequestRate = metricRegistry.rate("total_requests_rate");
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) {
        request.setAttribute(METRICS_START_TIME_ATTRIBUTE, Instant.now());
        return true;
    }

    @Override
    public void afterCompletion(HttpServletRequest request, HttpServletResponse response, Object handler,
                                @Nullable Exception ex) {
        try {
            allRequestRate.inc();
            Instant startTimeInstant = (Instant) request.getAttribute(METRICS_START_TIME_ATTRIBUTE);

            if (startTimeInstant != null) {
                if (handler instanceof HandlerMethod) {
                    String path = getPath(request.getServletPath());
                    RequestSensors sensors = requestCounterCache
                            .computeIfAbsent(path, key -> RequestSensors.withLabels(
                                    metricRegistry,
                                    Labels.of("path", path),
                                    "http.in.",
                                    histogramCollectorSupplier));
                    sensors.getRequestRate().inc();

                    if (ex != null || response.getStatus() >= 400) {
                        sensors.getFailureRate().inc();
                    }

                    var duration = Duration.between(startTimeInstant, Instant.now());
                    sensors.getRequestTimings().record(duration.toMillis());
                }
            }
        } catch (Exception e) {
            logger.error("ProcessingTimeInterceptor error", e);
        }
    }

    private String getPath(String servletPath) {
        return signalNamePattern.matcher(
                trimTrailingCharacter(trimLeadingCharacter(servletPath, '/'), '/')
        ).replaceAll("_");
    }
}
