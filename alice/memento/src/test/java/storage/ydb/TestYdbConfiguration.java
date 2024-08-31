package ru.yandex.alice.memento.storage.ydb;

import com.google.common.base.Strings;
import com.yandex.ydb.core.auth.AuthProvider;
import com.yandex.ydb.core.auth.NopAuthProvider;
import com.yandex.ydb.core.auth.TokenAuthProvider;
import com.yandex.ydb.core.grpc.GrpcTransport;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Primary;

@TestConfiguration
public class TestYdbConfiguration {
    private static final Logger logger = LogManager.getLogger();

    @Value("${ydb.endpoint}")
    private String endpoint;

    @Value("${ydb.database}")
    private String database;

    @Value("${ydb.token:}")
    private String token;

    @Bean
    @Primary
    GrpcTransport testGrpcTransport() {
        AuthProvider provider;

        if (!Strings.isNullOrEmpty(token)) {
            logger.info("ydb config - use token auth provider");
            provider = new TokenAuthProvider(token);
        } else {
            logger.info("ydb config - use Noop auth provider");
            provider = NopAuthProvider.INSTANCE;
        }
        return GrpcTransport.forEndpoint(endpoint, database)
                .withAuthProvider(provider)
                .build();
    }
}
