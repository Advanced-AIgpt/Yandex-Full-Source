package ru.yandex.alice.paskill.dialogovo.providers.recommendation;

import java.util.Map;

import com.google.common.collect.ImmutableMap;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;

@Configuration
class RecommendationProviderConfiguration {

    private static final Map<String, String> GAMES_ONBOARDING_SKILLS_MAPPING = ImmutableMap
            .<String, String>builder()
            .put("games_onboarding_words_in_word", "cd3875e6-9114-484c-b944-c25affd1c7e6")
            .put("onboarding_recipe", "143a0e0d-789c-403a-aba2-f41a9a1c31ca")
            .put("games_onboarding_guess_actor", "d134a3d2-615b-40fb-a86d-f8fd397cb707")
            .put("onboarding_believe_or_not_wintergames", "28983c1c-42e4-4e5a-8629-0b9bf7eb89e1")
            .put("games_onboarding_what_comes_first", "f3861c39-63e3-403b-80de-91fc4376a5d7")
            .put("onboarding_market_present", "8262beae-e2be-4f4c-bbfc-8916b810f718")
            .put("onboarding_gc_skill", "bd7c3799-5947-41d0-b3d3-4a35de977111")
            .put("onboarding_gc_skill2", "bd7c3799-5947-41d0-b3d3-4a35de977111")
            .put("games_onboarding_believe_or_not", "28983c1c-42e4-4e5a-8629-0b9bf7eb89e1")
            .put("games_onboarding_magic_ball", "766d9008-aec5-412c-90ff-daa61cd45a5c")
            .put("onboarding_game_records", "1394673e-28b6-4465-9a2a-b5b54c984c86")
            .put("onboarding_toast2", "7a62d38d-db2c-4a4a-bd63-4f6afebd68d5")
            .put("guess_the_song_15to18", "96287bd7-c9b2-4efd-b17a-fea29d6b40fb")
            .put("onboarding_music_fairy_tale_new_year", "bd168a52-c2eb-43be-a3df-632e207cd02b")
            .put("games_onboarding_guess_the_song", "96287bd7-c9b2-4efd-b17a-fea29d6b40fb")
            .put("games_onboarding_quest", "2f3c5214-bc3e-4bd2-9ae9-ff39d286f1ae")
            .put("cities_15to18", "672f7477-d3f0-443d-9bd5-2487ab0b6a4c")
            .put("games_onboarding_records", "1394673e-28b6-4465-9a2a-b5b54c984c86")
            .put("onboarding_congrats", "414ad9e7-47b9-4147-b4a5-0b289900581a")
            .put("onboarding_music_fairy_tale", "bd168a52-c2eb-43be-a3df-632e207cd02b")
            .put("games_onboarding_music_fairy_tale", "bd168a52-c2eb-43be-a3df-632e207cd02b")
            .put("music_fairy_tale_18to21", "bd168a52-c2eb-43be-a3df-632e207cd02b")
            .put("believe_or_not_15to18", "28983c1c-42e4-4e5a-8629-0b9bf7eb89e1")
            .put("games_onboarding_find_extra", "0d66ad2c-0e2e-43e1-bff0-c81de3df260a")
            .put("games_onboarding_cities", "672f7477-d3f0-443d-9bd5-2487ab0b6a4c")
            .put("games_onboarding_this_day_in_history", "2cd249d3-fe39-4a6b-8031-c094aca2fd6c")
            .put("games_onboarding_lao_wai", "8197850d-9305-4f63-9104-6a5cea388f4a")
            .put("onboarding_market_present2", "8262beae-e2be-4f4c-bbfc-8916b810f718")
            .put("games_onboarding_disney_monsters", "32168fc9-5d95-4057-a516-2afe762953e2")
            .put("games_onboarding_zoology", "35c04c96-5192-4e55-8366-15519b438184")
            .put("games_onboarding_divination", "76b050d4-ec02-4b2e-8b97-6525a93c276c")
            .put("onboarding_music_fairy_tale2", "bd168a52-c2eb-43be-a3df-632e207cd02b")
            .put("onboarding_toast", "7a62d38d-db2c-4a4a-bd63-4f6afebd68d5")
            .put("onboarding_toast_new_year", "7a62d38d-db2c-4a4a-bd63-4f6afebd68d5")
            .build();

    @Bean("gamesOnboardingWithRedefinedSkillIdsProvider")
    public SkillProviderWithRedefinedSkillIds gamesWithRedefinedSkillIdsRecommendationsProvider(
            SkillProvider skillProvider) {
        return new SkillProviderWithRedefinedSkillIds(GAMES_ONBOARDING_SKILLS_MAPPING, skillProvider);
    }
}
