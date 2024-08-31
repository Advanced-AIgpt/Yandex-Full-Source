package ru.yandex.quasar.billing.dao;

import java.io.IOException;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.sql.DataSource;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.jdbc.core.convert.JdbcCustomConversions;
import org.springframework.data.jdbc.repository.config.AbstractJdbcConfiguration;
import org.springframework.data.jdbc.repository.config.EnableJdbcRepositories;
import org.springframework.data.relational.core.dialect.Dialect;
import org.springframework.data.relational.core.dialect.PostgresDialect;
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcOperations;
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate;
import org.springframework.transaction.annotation.EnableTransactionManagement;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.DeliveryInfo;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

/**
 * Conversions configuration file. Conversions are used in Spring Data JDBC to map to/from entities
 */
@Configuration
@EnableJdbcRepositories(basePackages = "ru.yandex.quasar.billing.dao")
@EnableTransactionManagement(proxyTargetClass = true)
public class DaoConfig extends AbstractJdbcConfiguration {

    private final ObjectMapper objectMapper;
    private final DataSource dataSource;

    public DaoConfig(ObjectMapper objectMapper,
                     DataSource dataSource) {
        this.objectMapper = objectMapper;
        this.dataSource = dataSource;
    }

    @Bean
    public NamedParameterJdbcTemplate namedParameterJdbcTemplate() {
        return new NamedParameterJdbcTemplate(dataSource);
    }

    @Override
    public Dialect jdbcDialect(NamedParameterJdbcOperations operations) {
        return PostgresDialect.INSTANCE;
    }

    @Nonnull
    @Override
    public JdbcCustomConversions jdbcCustomConversions() {


        return new JdbcCustomConversions(List.<Converter>of(
                new Converter<ProviderContentItem, String>() {
                    @Override
                    public String convert(ProviderContentItem source) {
                        return DaoConfig.this.jsonToString(source);
                    }
                },
                new Converter<PricingOption, String>() {
                    @Override
                    public String convert(PricingOption source) {
                        return DaoConfig.this.jsonToString(source);
                    }
                },
                new Converter<String, ProviderContentItem>() {
                    @Override
                    public ProviderContentItem convert(String source) {
                        return DaoConfig.this.stringToJsonObject(source, ProviderContentItem.class);
                    }
                },
                new Converter<String, PricingOption>() {
                    @Override
                    public PricingOption convert(String source) {
                        return DaoConfig.this.stringToJsonObject(source, PricingOption.class);
                    }
                },
                new Converter<String, PurchaseOffer.PurchaseOfferOptions>() {
                    @Override
                    public PurchaseOffer.PurchaseOfferOptions convert(String source) {
                        return DaoConfig.this.stringToJsonObject(source, PurchaseOffer.PurchaseOfferOptions.class);
                    }
                },
                new Converter<PurchaseOffer.PurchaseOfferOptions, String>() {
                    @Override
                    public String convert(PurchaseOffer.PurchaseOfferOptions source) {
                        return DaoConfig.this.jsonToString(source);
                    }
                },
                new Converter<String, ContentItem>() {
                    @Override
                    public ContentItem convert(String source) {
                        return DaoConfig.this.stringToJsonObject(source, ContentItem.class);
                    }
                },
                new Converter<ContentItem, String>() {
                    @Override
                    public String convert(ContentItem source) {
                        return DaoConfig.this.jsonToString(source);
                    }
                },
                new Converter<String, DeliveryInfo>() {
                    @Override
                    public DeliveryInfo convert(String source) {
                        return DaoConfig.this.stringToJsonObject(source, DeliveryInfo.class);
                    }
                },
                new Converter<DeliveryInfo, String>() {
                    @Override
                    public String convert(DeliveryInfo source) {
                        return DaoConfig.this.jsonToString(source);
                    }
                }

        ));
    }

    @Nullable
    private <T> String jsonToString(@Nullable T source) {
        if (source == null) {
            return null;
        }
        try {
            return objectMapper.writeValueAsString(source);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    @Nullable
    private <T> T stringToJsonObject(@Nullable String source, Class<T> clazz) {
        if (source == null || source.isEmpty()) {
            return null;
        }
        try {
            return objectMapper.readValue(source, clazz);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

}
