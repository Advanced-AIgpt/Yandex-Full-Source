package ru.yandex.alice.paskills.my_alice.bunker.main_promo;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskills.my_alice.bunker.client.BunkerClient;
import ru.yandex.alice.paskills.my_alice.bunker.image_storage.ImageStorage;

@Configuration
public class MainPromoConfiguration {
    @Bean
    MainPromo mainPromo(
            BunkerClient bunkerClient,
            @Value("${BUNKER_ROOT:/my-alice/stable}") String projectRoot,
            ImageStorage imageStorage
    ) {
        return new MainPromoImpl(bunkerClient, projectRoot, imageStorage, true);
    }
}
