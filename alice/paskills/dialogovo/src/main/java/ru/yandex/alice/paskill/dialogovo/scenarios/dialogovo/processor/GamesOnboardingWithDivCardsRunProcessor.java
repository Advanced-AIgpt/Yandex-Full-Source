package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Image;
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillRecommendation;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillProviderWithRedefinedSkillIds;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RecommendationCardsRenderer;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillRecommendationsAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderCardName;
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderRequestAttributes;
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderResponse;
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderService;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static java.util.function.Predicate.not;

@Component
public class GamesOnboardingWithDivCardsRunProcessor implements SingleSemanticFrameRunProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();
    public static final int MIN_RECOMMENDATIONS_SIZE = 3;
    public static final int MAX_RECOMMENDATIONS_SIZE = 5;

    private final RecommenderService recommenderService;
    private final Rate tooFewRecommendationsRate;
    private final RecommendationCardsRenderer recommendationCardsRenderer;
    private final SkillProviderWithRedefinedSkillIds skillProvider;
    private final SuggestButtonFactory suggestButtonFactory;
    private final Phrases phrases;
    private final String avatarsUrl;
    private final String onboardingUrl;

    @SuppressWarnings("ParameterNumber")
    public GamesOnboardingWithDivCardsRunProcessor(
            MetricRegistry metricRegistry,
            RecommenderService recommenderService,
            RecommendationCardsRenderer recommendationCardsRenderer,
            @Qualifier("gamesOnboardingWithRedefinedSkillIdsProvider") SkillProviderWithRedefinedSkillIds skillProvider,
            SuggestButtonFactory suggestButtonFactory,
            Phrases phrases,
            @Value("${avatarsUrl}") String avatarsUrl,
            @Value("${gamesOnboardingConfig.storeOnboardingGamesUrl}") String onboardingUrl
    ) {
        this.recommenderService = recommenderService;
        this.tooFewRecommendationsRate = metricRegistry.rate("recommendations.games.onboarding.few");
        this.recommendationCardsRenderer = recommendationCardsRenderer;
        this.skillProvider = skillProvider;
        this.suggestButtonFactory = suggestButtonFactory;
        this.phrases = phrases;
        this.avatarsUrl = avatarsUrl;
        this.onboardingUrl = onboardingUrl;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return hasExperiment(Experiments.DIALOGOVO_DIV_CARDS_SURFACE_GAMES_ONBOARDING)
                .and(SURFACE_SUPPORTS_DIV_CARDS)
                .and(not(IS_IN_SKILL))
                .and(hasFrame(SemanticFrames.GAMES_ONBOARDING))
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.GAMES_ONBOARDING_WITH_DIV_CARDS;
    }

    @Override
    public String getSemanticFrame() {
        return SemanticFrames.GAMES_ONBOARDING;
    }

    @Override
    //TODO: add ElariWatch case
    //TODO: add Navigator(not smart_speaker && not div_cards && not elari) with simple text suggest case
    //TODO: add DJ Fallback response with predefined list of games(onboarding__default_card) + search suggest
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {

        RecommenderResponse recommenderResponse = recommenderService.search(RecommenderCardName.GAMES_ONBOARDING,
                null, convertRequestToRecommenderAttributes(request));

        List<SkillRecommendation> recommenderItems = filterRecommenderItems(recommenderResponse, request);

        if (recommenderItems.size() < MIN_RECOMMENDATIONS_SIZE) {
            logger.info("Recommender games onboarding has less than min required results. Return irrelevant");
            tooFewRecommendationsRate.inc();
            return DefaultIrrelevantResponse.create(Intents.IRRELEVANT, request.getRandom(), request.isVoiceSession());
        }

        context.getAnalytics().setIntent(Intents.PERSONAL_ASSISTANT_SKILL_RECOMMENDATION);
        context.getAnalytics().addObject(new SkillRecommendationsAnalyticsInfoObject(
                recommenderItems.stream().map(item -> item.getSkill().getId()).collect(Collectors.toList()),
                RecommendationType.GAMES_ONBOARDING));

        RecommendationCardsRenderer.SkillsRendererResult skillsRendererResult =
                recommendationCardsRenderer.renderSkillRecommendationCardClassic(
                        RecommendationCardsRenderer.RecommendationCardsRendererContext.builder()
                                .items(recommenderItems)
                                .activationSourceType(ActivationSourceType.GAMES_ONBOARDING)
                                .storeUrl(onboardingUrl)
                                .requestId(request.getRequestId())
                                .recommendationSource(recommenderResponse.getRecommendationSource())
                                .recommendationType(RecommendationType.GAMES_ONBOARDING)
                                .recommendationSubType(recommenderResponse.getRecommendationType())
                                .addVoiceActivation(request.hasExperiment(
                                        Experiments.DIALOGOVO_DIV_CARDS_SURFACE_GAMES_ONBOARDING_SKILL_VOICE_BUTTONS))
                                .build());

        return new RunOnlyResponse<>(new ScenarioResponseBody<>(
                Layout.cardBuilder(
                        phrases.get("divs.games_onboarding.recommendation.message"), true,
                        skillsRendererResult.getDivBody())
                        .suggests(List.of(suggestButtonFactory.getGamesOnboaringSuggest()))
                        .build(),
                context.getAnalytics().toAnalyticsInfo(),
                false,
                skillsRendererResult.getActions()));
    }

    private List<SkillRecommendation> filterRecommenderItems(RecommenderResponse recommenderResponse,
                                                             MegaMindRequest<DialogovoState> request) {
        return recommenderResponse
                .getItems()
                .stream()
                .map(recommenderItem ->
                        skillProvider.getSkill(recommenderItem.getSkillId())
                                .filter(SkillFilters.VALID_FOR_RECOMMENDATIONS)
                                .filter(skillInfo ->
                                        SkillFilters.EXPLICIT_CONTENT_RECOMMENDATION_FILTER.test(request, skillInfo))
                                .map(skillInfo -> new SkillRecommendation(
                                        skillInfo,
                                        recommenderItem.getActivation(),
                                        Image.skillLogoUrl(recommenderItem.getLogoAvatarId(), avatarsUrl))))
                .flatMap(Optional::stream)
                .limit(MAX_RECOMMENDATIONS_SIZE)
                .collect(Collectors.toList());
    }

    private RecommenderRequestAttributes convertRequestToRecommenderAttributes(
            MegaMindRequest<DialogovoState> request
    ) {
        Set<String> experiments = new HashSet<>(request.getExperiments());
        return new RecommenderRequestAttributes(experiments,
                new RecommenderRequestAttributes.UserData(request.getClientInfo().getUuid())
        );
    }
}
