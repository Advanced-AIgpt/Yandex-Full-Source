package ru.yandex.alice.paskill.dialogovo.service.show.yt;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.ShowConfig;
import ru.yandex.alice.paskill.dialogovo.service.show.ShowEpisodeStoreDao;

@Deprecated(since = "PASKILLS-8263. New show logic stores episodes in YDB")
@Configuration
@ConditionalOnProperty(value = "scheduler.controllers.enable", havingValue = "true")
public class YtShowEpisodeConfiguration {

    private final SecretsConfig secretsConfig;
    private final ShowConfig showConfig;

    public YtShowEpisodeConfiguration(
            SecretsConfig secretsConfig,
            ShowConfig showConfig
    ) {
        this.secretsConfig = secretsConfig;
        this.showConfig = showConfig;
    }

    @Bean
    @Qualifier("yt")
    public ShowEpisodeStoreDao showEpisodeStoreDao(
            YtServiceClient ytServiceClient
    ) {
        return new YtEpisodeStoreDaoImpl(showConfig, ytServiceClient);
    }


    @Bean
    public YtServiceClient ytServiceClient() {
        return new YtServiceClient(secretsConfig, showConfig);
    }
}
