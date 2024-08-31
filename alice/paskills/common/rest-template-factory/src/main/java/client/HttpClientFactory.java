package ru.yandex.alice.paskills.common.resttemplate.factory.client;

import java.io.IOException;
import java.net.ConnectException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.UnknownHostException;
import java.security.KeyManagementException;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.time.Duration;
import java.util.Arrays;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BiFunction;
import java.util.function.Predicate;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLException;

import org.apache.http.Header;
import org.apache.http.HttpHeaders;
import org.apache.http.HttpHost;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.protocol.HttpClientContext;
import org.apache.http.config.Registry;
import org.apache.http.config.RegistryBuilder;
import org.apache.http.config.SocketConfig;
import org.apache.http.conn.socket.ConnectionSocketFactory;
import org.apache.http.conn.socket.PlainConnectionSocketFactory;
import org.apache.http.conn.ssl.NoopHostnameVerifier;
import org.apache.http.conn.ssl.SSLConnectionSocketFactory;
import org.apache.http.conn.ssl.TrustAllStrategy;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.DefaultHttpRequestRetryHandler;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.impl.conn.PoolingHttpClientConnectionManager;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestExecutor;
import org.apache.http.ssl.SSLContextBuilder;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponents;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;


@Component
public class HttpClientFactory {

    private final MetricRegistry metricRegistry;

    public HttpClientFactory(
            MetricRegistry metricRegistry
    ) {
        this.metricRegistry = metricRegistry;
    }

    private static int intMillis(long millis) {
        return (int) (Math.min(millis, Integer.MAX_VALUE));
    }

    public Builder builder() {
        return new Builder(HttpClientBuilder.create(), true);
    }

    private static final class FixedNameInstrumentFactory implements BiFunction<HttpRequest, HttpContext, Instrument> {

        private final Instrument instrument;

        private FixedNameInstrumentFactory(Instrument instrument) {
            this.instrument = instrument;
        }

        @Override
        public Instrument apply(HttpRequest httpRequest, HttpContext context) {
            return instrument;
        }
    }

    private static final class CachedPoolInstrumentFactory implements BiFunction<HttpRequest, HttpContext, Instrument> {

        private final Map<String, Instrument> instrumentsCache = new ConcurrentHashMap<>();
        private final NamedSensorsRegistry namedMetricRegistry;

        private CachedPoolInstrumentFactory(NamedSensorsRegistry namedMetricRegistry) {
            this.namedMetricRegistry = namedMetricRegistry;
        }

        @Override
        public Instrument apply(HttpRequest httpRequest, HttpContext context) {
            String path = getPath(httpRequest);
            String host = getHost(httpRequest, context);

            return instrumentsCache.computeIfAbsent(
                    host + "/" + path,
                    empty -> namedMetricRegistry
                            .withLabels(Labels.of("url_host", host))
                            .withLabels(Labels.of("url_path", path))
                            .instrument("requests")
            );
        }

        private String getHost(HttpRequest httpRequest, HttpContext context) {
            String targetHost = getTargetHost(httpRequest, context);
            targetHost = "[::1]".equals(targetHost) ? "localhost" : targetHost;

            return targetHost;
        }

        private String getTargetHost(HttpRequest request, HttpContext context) {
            Optional<String> host = Optional.ofNullable(request.getRequestLine().getUri())
                    .map(uri -> UriComponentsBuilder.fromPath(uri).build())
                    .map(UriComponents::getHost);

            if (host.isPresent()) {
                return host.get();
            }

            if (context instanceof HttpClientContext) {
                return Optional.ofNullable(((HttpClientContext) context)
                                .getTargetHost())
                        .map(HttpHost::getHostName)
                        .orElse("localhost");
            }
            return Optional.ofNullable(request
                            .getFirstHeader(HttpHeaders.HOST)).map(Header::getValue)
                    .orElse("localhost");
        }

        private String getPath(HttpRequest request) {
            return Optional.ofNullable(request.getRequestLine().getUri())
                    .flatMap(uri -> {
                        try {
                            return Optional.of(new URI(uri));
                        } catch (URISyntaxException e) {
                            return Optional.empty();
                        }
                    })
                    .map(URI::getPath)
                    .orElse("/");
        }
    }

    private static final class InterruptedIOHttpRequestRetryHandler extends DefaultHttpRequestRetryHandler {

        InterruptedIOHttpRequestRetryHandler(int retryCount, boolean requestSentRetryEnabled) {
            // Do not retry on UnknownHostException, ConnectException, SSLException
            // Do retry on InterruptedIOException though, it may indicate socket timeout condition
            super(retryCount, requestSentRetryEnabled, Arrays.asList(
                    UnknownHostException.class,
                    ConnectException.class
                    // Socket exception is wrapped into SSL exception and may be retried.
                    // see paste in https://st.yandex-team.ru/PASKILLS-5939
                    //,SSLException.class
            ));
        }

        @Override
        public boolean retryRequest(IOException exception, int executionCount, HttpContext context) {
            IOException cause = exception;
            if (exception instanceof SSLException) {
                if (exception.getCause() != null && exception.getCause() instanceof IOException) {
                    cause = (IOException) exception.getCause();
                }
            }
            return super.retryRequest(cause, executionCount, context);
        }
    }

    public final class Builder {
        private final PoolingHttpClientConnectionManager connManager;
        private final RequestConfig.Builder requestConfigBuilder;
        private final SocketConfig.Builder defaultSocketConfigBuilder;
        private final HttpClientBuilder httpClientBuilder;
        private HttpRequestExecutor requestExecutor;
        private int minFailureHttpStatus = HttpStatus.SC_BAD_REQUEST;
        private Duration validateAfterInactivityPeriod = Duration.ofSeconds(1);
        private Optional<Predicate<HttpResponse>> responseRetryRuleO = Optional.empty();

        public Builder(HttpClientBuilder httpClientBuilder, boolean validateSsl) {
            var sslConnectionSocketFactory = !validateSsl ?
                    novalidateSslConnectionSocketFactory() :
                    SSLConnectionSocketFactory.getSocketFactory();
            Registry<ConnectionSocketFactory> registry = RegistryBuilder.<ConnectionSocketFactory>create()
                    .register("http", PlainConnectionSocketFactory.getSocketFactory())
                    .register("https", sslConnectionSocketFactory)
                    .build();
            this.connManager = new PoolingHttpClientConnectionManager(registry);
            this.requestConfigBuilder = RequestConfig.custom();
            this.defaultSocketConfigBuilder = SocketConfig.custom();
            this.httpClientBuilder = httpClientBuilder;
            this.requestExecutor = new HttpRequestExecutor();
        }

        private SSLConnectionSocketFactory novalidateSslConnectionSocketFactory() {
            try {
                SSLContext sslContext = SSLContextBuilder.create()
                        .loadTrustMaterial(null, TrustAllStrategy.INSTANCE)
                        .build();
                return new SSLConnectionSocketFactory(sslContext, NoopHostnameVerifier.INSTANCE);
            } catch (NoSuchAlgorithmException | KeyStoreException | KeyManagementException e) {
                throw new RuntimeException(e);
            }
        }

        public Builder connectTimeout(Duration timeout) {
            int timeoutMs = intMillis(timeout.toMillis());
            requestConfigBuilder
                    .setConnectionRequestTimeout(timeoutMs)
                    .setConnectTimeout(timeoutMs)
                    .setSocketTimeout(timeoutMs);
            defaultSocketConfigBuilder
                    .setSoTimeout(timeoutMs);
            return this;
        }

        public Builder connectionRequestTimeout(Duration connectionRequestTimeout) {
            int timeoutMs = intMillis(connectionRequestTimeout.toMillis());
            requestConfigBuilder.setConnectionRequestTimeout(timeoutMs);
            return this;
        }

        public Builder socketTimeout(Duration socketTimeout) {
            int timeoutMs = intMillis(socketTimeout.toMillis());
            requestConfigBuilder.setSocketTimeout(timeoutMs);
            defaultSocketConfigBuilder.setSoTimeout(timeoutMs);
            return this;
        }

        public Builder maxConnectionCount(int maxConnectionCount) {
            connManager.setDefaultMaxPerRoute(maxConnectionCount);
            connManager.setMaxTotal(maxConnectionCount);
            return this;
        }

        public Builder validateAfterInactivityPeriod(Duration validateAfterInactivityPeriod) {
            this.validateAfterInactivityPeriod = validateAfterInactivityPeriod;
            return this;
        }

        public Builder maxConnectionsTotal(int maxConnectionsTotal) {
            connManager.setMaxTotal(maxConnectionsTotal);
            return this;
        }

        public Builder maxConnectionsPerRoute(int maxConnectionsPerRoute) {
            connManager.setDefaultMaxPerRoute(maxConnectionsPerRoute);
            return this;
        }

        public Builder userAgent(String userAgent) {
            httpClientBuilder.setUserAgent(userAgent);
            return this;
        }

        public Builder cookieSpec(String cookieSpec) {
            requestConfigBuilder.setCookieSpec(cookieSpec);
            return this;
        }

        public Builder httpProxy(String hostname, int port) {
            return proxy(hostname, port, HttpHost.DEFAULT_SCHEME_NAME);
        }

        public Builder proxy(final String hostname, final int port, String scheme) {
            requestConfigBuilder.setProxy(new HttpHost(hostname, port, scheme));
            return this;
        }

        public Builder retryOnIO(int retryCount, boolean requestSentRetryEnabled) {
            httpClientBuilder.setRetryHandler(new InterruptedIOHttpRequestRetryHandler(retryCount,
                    requestSentRetryEnabled));
            return this;
        }

        public Builder retryByIOAndRule(int retryCount, boolean requestSentRetryEnabled,
                                        Predicate<HttpResponse> responseRetryRule) {
            httpClientBuilder.setRetryHandler(new InterruptedIOHttpRequestRetryHandler(retryCount,
                    requestSentRetryEnabled));
            this.responseRetryRuleO = Optional.of(responseRetryRule);
            return this;
        }

        public Builder retryByIOAnd5XX(int retryCount, boolean requestSentRetryEnabled) {
            return retryByIOAndRule(retryCount, requestSentRetryEnabled,
                    httpResponse -> {
                        int statusCode = httpResponse.getStatusLine().getStatusCode();
                        return statusCode >= 500 && statusCode <= 599;

                    });
        }

        public Builder disableRedirects() {
            httpClientBuilder.disableRedirectHandling();
            return this;
        }

        public Builder disableCookieManagement() {
            httpClientBuilder.disableCookieManagement();
            return this;
        }

        public Builder disableRetry() {
            httpClientBuilder.disableAutomaticRetries();
            return this;
        }

        public Builder minFailureHttpStatus(int minFailureHttpStatus) {
            this.minFailureHttpStatus = minFailureHttpStatus;
            return this;
        }

        public Builder instrumented(String prefix, String serviceName, boolean withPath,
                                    Optional<Duration> logWarnThresholdO) {
            NamedSensorsRegistry namedMetricRegistry = new NamedSensorsRegistry(
                    metricRegistry.subRegistry("target", serviceName),
                    prefix);

            // http pool stats
            NamedSensorsRegistry poolMetricRegistry = namedMetricRegistry.sub("pool");
            HttpClientPoolMetrics poolMetrics = new HttpClientPoolMetrics(connManager);
            poolMetricRegistry.lazyGauge("available", poolMetrics::available);
            poolMetricRegistry.lazyGauge("leased", poolMetrics::leased);
            poolMetricRegistry.lazyGauge("max", poolMetrics::max);
            poolMetricRegistry.lazyGauge("pending", poolMetrics::pending);

            // http requests stats
            BiFunction<HttpRequest, HttpContext, Instrument> instrumentSupplier;
            if (withPath) {
                instrumentSupplier = new CachedPoolInstrumentFactory(namedMetricRegistry);
            } else {
                instrumentSupplier = new FixedNameInstrumentFactory(namedMetricRegistry.instrument("requests"));
            }

            this.requestExecutor = new InstrumentedRequestExecutor(
                    minFailureHttpStatus,
                    instrumentSupplier,
                    logWarnThresholdO,
                    responseRetryRuleO,
                    serviceName);
            return this;
        }

        public CloseableHttpClient build() {
            RequestConfig requestConfig = requestConfigBuilder.build();
            SocketConfig socketConfig = defaultSocketConfigBuilder.build();
            connManager.setDefaultSocketConfig(socketConfig);
            connManager.setValidateAfterInactivity(intMillis(validateAfterInactivityPeriod.toMillis()));

            return httpClientBuilder
                    .setDefaultRequestConfig(requestConfig)
                    .setConnectionManager(connManager)
                    .setRequestExecutor(requestExecutor)
                    .build();
        }
    }
}
