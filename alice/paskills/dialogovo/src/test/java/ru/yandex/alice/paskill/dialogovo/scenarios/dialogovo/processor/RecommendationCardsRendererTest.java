package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;


import java.util.List;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.json.JSONException;
import org.junit.jupiter.api.Test;
import org.skyscreamer.jsonassert.JSONAssert;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Image;
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillRecommendation;
import ru.yandex.alice.paskill.dialogovo.scenarios.RecommendationCardsRenderer;
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderResponse;
import ru.yandex.alice.paskill.dialogovo.test.TestSkills;
import ru.yandex.alice.paskill.dialogovo.utils.ResourceUtils;

@SpringBootTest(classes = {TestConfigProvider.class})
class RecommendationCardsRendererTest {

    @Autowired
    private ObjectMapper objectMapper;

    @Autowired
    private RecommendationCardsRenderer renderer;

    @Value("${avatarsUrl}")
    private String avatarsUrl;

    @Value("${gamesOnboardingConfig.storeOnboardingGamesUrl}")
    private String onboardingUrl;

    private static final String REQUEST_ID = "16b44a33-397c-4286-b26a-48ff36f3ce50";

    @Test
    public void renderSkillRecommendationCardClassic() throws JsonProcessingException, JSONException {
        RecommenderResponse recommenderResponse = objectMapper.readValue(ResourceUtils.getStringResource(
                        "renderer/games_onboarding_renderer_response.json"),
                RecommenderResponse.class);

        RecommendationCardsRenderer.SkillsRendererResult skillsRendererResult =
                renderer.renderSkillRecommendationCardClassic(
                        RecommendationCardsRenderer.RecommendationCardsRendererContext
                                .builder()
                                .requestId(REQUEST_ID)
                                .items(convert(recommenderResponse))
                                .activationSourceType(ActivationSourceType.GAMES_ONBOARDING)
                                .storeUrl(onboardingUrl)
                                .recommendationSource(recommenderResponse.getRecommendationSource())
                                .recommendationSubType(recommenderResponse.getRecommendationType())
                                .recommendationType(RecommendationType.GAMES_ONBOARDING)
                                .build());

        JSONAssert.assertEquals(
                ResourceUtils.getStringResource("renderer/games_onboarding_card_result.json"),
                objectMapper.writeValueAsString(skillsRendererResult),
                JSONCompareMode.STRICT
        );
    }

    @Test
    public void renderSkillRecommendationCardClassicWithVoiceActions() throws JsonProcessingException, JSONException {
        RecommenderResponse recommenderResponse = objectMapper.readValue(ResourceUtils.getStringResource(
                        "renderer/games_onboarding_renderer_response.json"),
                RecommenderResponse.class);

        RecommendationCardsRenderer.SkillsRendererResult skillsRendererResult =
                renderer.renderSkillRecommendationCardClassic(
                        RecommendationCardsRenderer.RecommendationCardsRendererContext
                                .builder()
                                .requestId(REQUEST_ID)
                                .items(convert(recommenderResponse))
                                .activationSourceType(ActivationSourceType.GAMES_ONBOARDING)
                                .storeUrl(onboardingUrl)
                                .recommendationSource(recommenderResponse.getRecommendationSource())
                                .recommendationSubType(recommenderResponse.getRecommendationType())
                                .recommendationType(RecommendationType.GAMES_ONBOARDING)
                                .addVoiceActivation(true)
                                .build());

        JSONAssert.assertEquals(
                ResourceUtils.getStringResource("renderer/games_onboarding_card_result_with_voice_buttons.json"),
                objectMapper.writeValueAsString(skillsRendererResult),
                JSONCompareMode.STRICT
        );
    }

    @Test
    public void renderSkillRecommendationCardClassicFromImplicitDiscoveryWithVoiceActions()
            throws JsonProcessingException, JSONException {
        List<SkillRecommendation> discoveryRecommendations = List.of(new SkillRecommendation(
                objectMapper.readValue(ResourceUtils.getStringResource("renderer/implicit_discovery_skill.json"),
                        SkillInfo.class), "", ""));

        RecommendationCardsRenderer.SkillsRendererResult skillsRendererResult =
                renderer.renderSkillRecommendationCardClassic(
                        RecommendationCardsRenderer.RecommendationCardsRendererContext
                                .builder()
                                .requestId(REQUEST_ID)
                                .items(discoveryRecommendations)
                                .activationSourceType(ActivationSourceType.DISCOVERY)
                                .storeUrl(onboardingUrl)
                                .recommendationSource("")
                                .recommendationSubType("")
                                .recommendationType(RecommendationType.SKILLS_IMPLICIT_DISCOVERY)
                                .addVoiceActivation(true)
                                .build());

        JSONAssert.assertEquals(
                ResourceUtils.getStringResource("renderer/implicit_skill_discovery_card_result.json"),
                objectMapper.writeValueAsString(skillsRendererResult),
                JSONCompareMode.STRICT
        );
    }

    private List<SkillRecommendation> convert(RecommenderResponse recommenderResponse) {
        return recommenderResponse.getItems()
                .stream()
                .map(recommenderItem ->
                        new SkillRecommendation(
                                TestSkills.cityGameSkill(),
                                recommenderItem.getActivation(),
                                Image.skillLogoUrl(recommenderItem.getLogoAvatarId(), avatarsUrl)))
                .collect(Collectors.toList());
    }
}
