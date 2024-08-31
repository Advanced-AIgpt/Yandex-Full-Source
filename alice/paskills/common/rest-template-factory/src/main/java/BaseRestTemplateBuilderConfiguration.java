package ru.yandex.alice.paskills.common.resttemplate.factory;

import java.util.List;
import java.util.stream.Collectors;

import org.springframework.beans.factory.ObjectProvider;
import org.springframework.boot.autoconfigure.http.HttpMessageConverters;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.client.RestTemplateCustomizer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;
import org.springframework.util.CollectionUtils;

@Configuration
public class BaseRestTemplateBuilderConfiguration {

    private final RestTemplateBuilder baseBuilder;

    public BaseRestTemplateBuilderConfiguration(
            ObjectProvider<RestTemplateCustomizer> restTemplateCustomizers,
            ObjectProvider<HttpMessageConverters> messageConverters) {

        RestTemplateBuilder builder = new RestTemplateBuilder();
        HttpMessageConverters converters = messageConverters.getIfUnique();
        if (converters != null) {
            builder = builder.messageConverters(converters.getConverters());
        }

        List<RestTemplateCustomizer> customizers = restTemplateCustomizers
                .orderedStream().collect(Collectors.toList());
        if (!CollectionUtils.isEmpty(customizers)) {
            builder = builder.customizers(customizers);
        }
        this.baseBuilder = builder;
    }

    @Primary
    @Bean("baseRestTemplateBuilder")
    public RestTemplateBuilder baseRestTemplateBuilder() {
        return baseBuilder;
    }
}
