package ru.yandex.alice.paskills.my_alice.bunker.client;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class BunkerClientConfiguration {
    @Bean
    BunkerClient bunker(
            @Value("${BUNKER_API:http://bunker-api.yandex.net}") String bunkerApi,
            @Value("${BUNKER_NODE_VERSION:stable}") String defaultNodeVersion
    ) {
        return new BunkerClientImpl(
                bunkerApi,
                defaultNodeVersion
        );
    }
}
