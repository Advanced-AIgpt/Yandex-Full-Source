package ru.yandex.quasar.billing.filter;

import java.time.Duration;
import java.util.List;
import java.util.stream.Collectors;

import org.apache.http.client.HttpClient;
import org.apache.http.config.Registry;
import org.apache.http.config.RegistryBuilder;
import org.apache.http.conn.socket.ConnectionSocketFactory;
import org.apache.http.conn.socket.PlainConnectionSocketFactory;
import org.apache.http.conn.ssl.SSLConnectionSocketFactory;
import org.apache.http.impl.client.DefaultClientConnectionReuseStrategy;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.impl.conn.PoolingHttpClientConnectionManager;
import org.springframework.beans.factory.ObjectProvider;
import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean;
import org.springframework.boot.autoconfigure.http.HttpMessageConverters;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.client.RestTemplateCustomizer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.client.BufferingClientHttpRequestFactory;
import org.springframework.http.client.ClientHttpRequestFactory;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.util.CollectionUtils;
import org.springframework.web.client.RestTemplate;

@Configuration
class RestTemplateProvider {

    private final YTLoggerHttpRequestInterceptor ytLoggingInterceptor;
    private final UnistatHttpRequestInterceptor unistatHttpRequestInterceptor;
    private final NginxHeadersInterceptor nginxHeadersInterceptor;
    private final ObjectProvider<HttpMessageConverters> messageConverters;
    private final ObjectProvider<RestTemplateCustomizer> restTemplateCustomizers;

    RestTemplateProvider(
            YTLoggerHttpRequestInterceptor ytLoggingInterceptor,
            UnistatHttpRequestInterceptor unistatHttpRequestInterceptor,
            NginxHeadersInterceptor nginxHeadersInterceptor,
            ObjectProvider<HttpMessageConverters> messageConverters,
            ObjectProvider<RestTemplateCustomizer> restTemplateCustomizers
    ) {
        this.ytLoggingInterceptor = ytLoggingInterceptor;
        this.unistatHttpRequestInterceptor = unistatHttpRequestInterceptor;
        this.nginxHeadersInterceptor = nginxHeadersInterceptor;
        this.messageConverters = messageConverters;
        this.restTemplateCustomizers = restTemplateCustomizers;
    }

    @Bean
    @ConditionalOnMissingBean
    public RestTemplateBuilder restTemplateBuilder() {
        RestTemplateBuilder builder = new RestTemplateBuilder();
        HttpMessageConverters converters = this.messageConverters.getIfUnique();
        if (converters != null) {
            builder = builder.messageConverters(converters.getConverters());
        }

        List<RestTemplateCustomizer> customizers = this.restTemplateCustomizers.orderedStream()
                .collect(Collectors.toList());
        if (!CollectionUtils.isEmpty(customizers)) {
            builder = builder.customizers(customizers);
        }

        /*
         * Using customizer we provider default {@link RestTemplateBuilder} configuration with logging and unistat and
         * nginx headers which may be used to create additionally configured {@link RestTemplate} with timeouts for
         * example.
         * If no additional configuration is needed, {@link #defaultRestTemplate(RestTemplateBuilder)} may be autowired.
         */
        return builder.requestFactory(this::requestFactory)
                .additionalInterceptors(
                        unistatHttpRequestInterceptor,
                        nginxHeadersInterceptor,
                        ytLoggingInterceptor
                );
    }

    public HttpClient httpClient() {
        Registry<ConnectionSocketFactory> registry = RegistryBuilder.<ConnectionSocketFactory>create()
                .register("http", PlainConnectionSocketFactory.getSocketFactory())
                .register("https", SSLConnectionSocketFactory.getSocketFactory())
                .build();

        var connManager = new PoolingHttpClientConnectionManager(registry);
        connManager.setMaxTotal(500);
        connManager.setDefaultMaxPerRoute(500);
        connManager.setValidateAfterInactivity(Math.toIntExact(Duration.ofSeconds(1).toMillis()));

        return HttpClientBuilder.create()
                .setConnectionManager(connManager)
                .setConnectionReuseStrategy(DefaultClientConnectionReuseStrategy.INSTANCE)
                .build();
    }

    private ClientHttpRequestFactory requestFactory() {
        var requestFactory = new HttpComponentsClientHttpRequestFactory(httpClient());
        requestFactory.setConnectionRequestTimeout(300);
        return new BufferingClientHttpRequestFactory(requestFactory);
    }

    @Bean
    public RestTemplate defaultRestTemplate(RestTemplateBuilder restTemplateBuilder) {
        return restTemplateBuilder.build();
    }
}
