package ru.yandex.alice.paskills.my_alice.bunker.image_storage;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskills.my_alice.bunker.client.BunkerClient;

@Configuration
public class ImageStorageConfiguration {
    @Bean
    ImageStorage imageStorage(
            BunkerClient bunkerClient,
            @Value("${BUNKER_ROOT:/my-alice/stable}") String projectRoot
    ) {
        return new ImageStorageImpl(bunkerClient, projectRoot, true);
    }
}
