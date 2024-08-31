package ru.yandex.alice.paskill.dialogovo.text;

import java.util.List;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;

import ru.yandex.alice.kronstadt.core.text.Phrases;

@Configuration
public class PhrasesConfiguration {

    @Bean
    @Primary
    public Phrases phrases() {
        return new Phrases(List.of(
                "phrases/alice4business",
                "phrases/feedback",
                "phrases/games_onboarding",
                "phrases/general",
                "phrases/suggests",
                "phrases/skill_activation_station",
                "phrases/skill_activation",
                "phrases/skill_product_activation",
                "phrases/sound_level",
                "phrases/flash_briefings/news",
                "phrases/flash_briefings/facts",
                "phrases/purchase_in_skills",
                "phrases/geolocation_sharing",
                "phrases/discovery",
                "phrases/user_agreements",
                "phrases/account_linking"
        ));
    }

}
