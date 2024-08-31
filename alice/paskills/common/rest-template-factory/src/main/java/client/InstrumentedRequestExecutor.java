package ru.yandex.alice.paskills.common.resttemplate.factory.client;

import java.io.IOException;
import java.time.Duration;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.function.BiFunction;
import java.util.function.Predicate;

import com.google.common.base.Stopwatch;
import org.apache.http.HttpClientConnection;
import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestExecutor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskills.common.solomon.utils.Instrument;

public final class InstrumentedRequestExecutor extends HttpRequestExecutor {
    private static final Logger logger = LogManager.getLogger("HttpRequestExecutor");

    private final BiFunction<HttpRequest, HttpContext, Instrument> instrumentSupplier;
    private final int minFailureHttpStatus;
    private final Optional<Duration> warnThresholdO;
    private final Optional<Predicate<HttpResponse>> responseRetryRule;
    private final String service;

    InstrumentedRequestExecutor(
            int minFailureHttpStatus,
            BiFunction<HttpRequest, HttpContext, Instrument> instrumentSupplier,
            Optional<Duration> logWarnThresholdO,
            Optional<Predicate<HttpResponse>> responseRetryRule,
            String serviceName) {
        this.instrumentSupplier = instrumentSupplier;
        this.minFailureHttpStatus = minFailureHttpStatus;
        this.warnThresholdO = logWarnThresholdO;
        this.responseRetryRule = responseRetryRule;
        this.service = serviceName;
    }

    @Override
    public HttpResponse execute(HttpRequest request, HttpClientConnection conn, HttpContext context)
            throws IOException, HttpException {

        Stopwatch timer = Stopwatch.createStarted();
        Instrument invocations = instrumentSupplier.apply(request, context);

        try {
            HttpResponse result = super.execute(request, conn, context);

            boolean success = isSuccess(result);
            invocations.measure(timer.elapsed(TimeUnit.MILLISECONDS), success);

            if (!success && responseRetryRule.isPresent() && responseRetryRule.get().test(result)) {
                throw new IOException("retry");
            }
            return result;
        } catch (Exception e) {
            invocations.measure(timer.elapsed(TimeUnit.MILLISECONDS), false);
            throw e;
        } finally {
            Duration elapsed = timer.elapsed();
            logger.debug("Http client call to service [{}] took {}ms", service, elapsed.toMillis());
            if (warnThresholdO.isPresent()) {
                if (elapsed.compareTo(warnThresholdO.get()) > 0) {
                    logger.warn("Http client call to service [{}] took {}ms (LONG)", service, elapsed.toMillis());
                }
            }
        }
    }

    private boolean isSuccess(HttpResponse result) {
        int statusCode = result.getStatusLine().getStatusCode();
        return statusCode < minFailureHttpStatus;
    }
}
