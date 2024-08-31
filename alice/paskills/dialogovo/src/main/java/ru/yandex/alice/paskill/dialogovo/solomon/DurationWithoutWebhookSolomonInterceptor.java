package ru.yandex.alice.paskill.dialogovo.solomon;

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

import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.springframework.util.StringUtils.trimLeadingCharacter;
import static org.springframework.util.StringUtils.trimTrailingCharacter;

@Component
public class DurationWithoutWebhookSolomonInterceptor implements HandlerInterceptor {
    private static final Logger logger = LogManager.getLogger();


    private final MetricRegistry metricRegistry;
    private final DialogovoRequestContext dialogovoRequestContext;
    private final ConcurrentMap<String, Histogram> durationWithoutWebhookCache = new ConcurrentHashMap<>();
    private final Supplier<HistogramCollector> histogramCollectorSupplier = () ->
            Histograms.explicit(10, 30, 60, 90, 120, 150, 200, 250, 260, 280, 290, 300, 310, 320, 400, 500, 1000,
                    1500, 2000, 2500, 2800, 2900, 3000, 3100, 3200, 3300, 3500, 4000);

    private static final String ATTRIBUTE = "ru.yandex.alice.paskill.dialogovo.solomon" +
            ".DurationWithoutWebhookSolomonInterceptor.START_TIME";
    private final Pattern signalNamePattern = Pattern.compile("[/\\-.]");

    public DurationWithoutWebhookSolomonInterceptor(MetricRegistry metricRegistry,
                                                    DialogovoRequestContext dialogovoRequestContext) {
        this.dialogovoRequestContext = dialogovoRequestContext;
        this.metricRegistry = metricRegistry;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) {
        request.setAttribute(ATTRIBUTE, Instant.now());
        return true;
    }

    @Override
    public void afterCompletion(HttpServletRequest request, HttpServletResponse response, Object handler,
                                @Nullable Exception ex) {
        try {
            Instant startTimeInstant = (Instant) request.getAttribute(ATTRIBUTE);

            if (startTimeInstant != null
                    && handler instanceof HandlerMethod
                    && dialogovoRequestContext.getWebhookRequestDurationMs() > 0) {

                String path = getPath(request.getServletPath());
                var totalDuration = Duration.between(startTimeInstant, Instant.now());

                // record only when skill is requested
                Histogram hist = durationWithoutWebhookCache.computeIfAbsent(path,
                        key -> metricRegistry.histogramRate(
                                "processing_without_webhook",
                                Labels.of("path", key),
                                histogramCollectorSupplier)
                );
                var dur = totalDuration.toMillis() - dialogovoRequestContext.getWebhookRequestDurationMs();
                hist.record(dur);

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
