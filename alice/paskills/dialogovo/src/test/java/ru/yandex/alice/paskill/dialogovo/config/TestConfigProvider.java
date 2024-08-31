package ru.yandex.alice.paskill.dialogovo.config;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import javax.sql.DataSource;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.jackson.JacksonAutoConfiguration;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Primary;
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcOperations;
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate;
import org.springframework.jdbc.datasource.DataSourceTransactionManager;
import org.springframework.jdbc.datasource.DelegatingDataSource;
import org.springframework.jdbc.datasource.DriverManagerDataSource;
import org.springframework.transaction.TransactionManager;

import ru.yandex.alice.paskill.dialogovo.tvm.UnitTestTvmClient;
import ru.yandex.passport.tvmauth.TvmClient;

@TestConfiguration("configProvider")
@Import({JacksonAutoConfiguration.class, DialogovoJdbcConfiguration.class})
public class TestConfigProvider {

    public static final String TVM_TOKEN = "tvmToken";
    public static final String CSRF_TOKEN = "1234567890";

    static {
        System.setProperty("CSRF_TOKEN_KEY", CSRF_TOKEN);
        System.setProperty("QLOUD_TVM_TOKEN", TVM_TOKEN);
    }

    @Autowired
    private ObjectMapper objectMapper;

    @Bean
    public SecretsConfig secretsConfig() throws IOException {

        String secrets = Files.readString(Path.of("config/dev/secrets.json"), StandardCharsets.UTF_8);

        return objectMapper.readValue(secrets, SecretsConfig.class);
    }


    @Bean
    @Primary
    public DataSource testDataSource() {
        return new DelegatingDataSource(
                new DriverManagerDataSource("jdbc:postgresql://localhost:0/dbname", "user", "pass")
        );
    }

    @Bean
    public TvmClient testTvmClient() {
        return new UnitTestTvmClient();
    }

    @Bean
    public NamedParameterJdbcOperations alice4businessJdbcOperations(DataSource testDataSource) {
        return new NamedParameterJdbcTemplate(testDataSource);
    }

    @Bean
    public TransactionManager alice4BusinessTransactionManager(DataSource testDataSource) {
        return new DataSourceTransactionManager(testDataSource);
    }
}
