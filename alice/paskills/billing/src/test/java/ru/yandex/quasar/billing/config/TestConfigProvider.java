package ru.yandex.quasar.billing.config;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

import javax.sql.DataSource;

import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean;
import org.springframework.boot.autoconfigure.jackson.JacksonAutoConfiguration;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Import;
import org.springframework.jdbc.core.JdbcTemplate;

import ru.yandex.alice.library.routingdatasource.WithDataSourceBeanPostProcessor;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.PostgresTestingConfiguration;
import ru.yandex.quasar.billing.PostgresTestingConfiguration.PostgresDataSourceConfig;
import ru.yandex.quasar.billing.dao.DaoConfig;
import ru.yandex.quasar.billing.dao.DatasourceProvider;
import ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;

import static java.util.stream.Collectors.toMap;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;


@TestConfiguration("configProvider")
@Import({
        DaoConfig.class,
        JacksonAutoConfiguration.class,
        PostgresTestingConfiguration.class,
        WithDataSourceBeanPostProcessor.class,
        DatasourceProvider.class
})
public class TestConfigProvider {

    public static final String TVM_TOKEN = "tvmToken";
    public static final String CSRF_TOKEN = "1234567890";

    static {
        System.setProperty("CSRF_TOKEN_KEY", CSRF_TOKEN);
        System.setProperty("QLOUD_TVM_TOKEN", TVM_TOKEN);
    }

    @Bean({"billingConfig", "testBillingConfig"})
    public BillingConfig billingConfig() throws IOException {
        String quasarHostRu = "localhost";
        String quasarHostNet = "localhost";

        String config = Files.readString(Path.of("configs/dev/quasar-billing.cfg"), StandardCharsets.UTF_8);

        config = config.replace("${QUASAR_HOST_RU}", quasarHostRu)
                .replace("${QUASAR_HOST_NET}", quasarHostNet);

        BillingConfig billingConfig = new ObjectMapper().readValue(config, BillingConfig.class);
        billingConfig.getTvmConfig().setTvmToken(TVM_TOKEN);
        return billingConfig;

    }

    @Bean
    public SecretsConfig secretsConfig(
            @Autowired(required = false) PostgresDataSourceConfig config
    ) throws IOException {

        String secrets = Files.readString(Path.of("configs/dev/secrets.cfg"), StandardCharsets.UTF_8);

        SecretsConfig secretsConfig = new ObjectMapper()
                .configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false)
                .readValue(secrets, SecretsConfig.class);
        if (config != null) {
            var jdbcUrl = config.url();
            secretsConfig = spy(secretsConfig);
            when(secretsConfig.getPgReadOnlyDatabaseUrl()).thenReturn(jdbcUrl);
            when(secretsConfig.getPgReadWriteDatabaseUrl()).thenReturn(jdbcUrl);
            when(secretsConfig.getPgPassword()).thenReturn(config.password());
            when(secretsConfig.getPgUser()).thenReturn(config.username());
        }

        secretsConfig.setTvmAliases(
                Arrays.stream(TvmClientName.values())
                        .collect(toMap(TvmClientName::getAlias, tvmClientName -> tvmClientName.ordinal()))
        );
        return secretsConfig;
    }


    @Bean
    public JdbcTemplate jdbcTemplate(DataSource dataSource) {
        return new JdbcTemplate(dataSource);
    }

    @Bean//("tvmServiceImpl")
    @ConditionalOnMissingBean(TvmClient.class)
    public TvmClient testTvmServiceImpl(SecretsConfig config) {
        return new TestTvmClientImpl(config);
    }

}
