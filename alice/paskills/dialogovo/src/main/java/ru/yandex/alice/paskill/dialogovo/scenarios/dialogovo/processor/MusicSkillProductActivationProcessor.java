package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.function.Predicate;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.StartMusicRecognizerDirective;
import ru.yandex.alice.kronstadt.core.input.Input;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.config.UserSkillProductConfig;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.ActivationType;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.ProductActivationState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static ru.yandex.alice.kronstadt.core.input.Input.Music.MusicResult.NOT_MUSIC;
import static ru.yandex.alice.kronstadt.core.input.Input.Music.MusicResult.SUCCESS;
import static ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor.getSkillId;
import static ru.yandex.alice.paskill.dialogovo.scenarios.Intents.MUSIC_SKILL_PRODUCT_ACTIVATION_RETRY;
import static ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType.MUSIC_SKILL_PRODUCT_ACTIVATION;

@Component
public class MusicSkillProductActivationProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private static final String RETRY_NOT_MUSIC_KEY = "skill_product.activation.handle_directive.retry.not_music";
    private static final String RETRY_DONT_MATCH_KEY = "skill_product.activation.handle_directive.retry.dont_match";
    private static final int MAX_MUSIC_ATTEMPT_COUNT = 1;

    private final SkillProvider skillProvider;
    private final MegaMindRequestSkillApplier skillApplier;
    private final Phrases phrases;
    private final UserSkillProductConfig skillProductConfig;
    private final MetricRegistry metricRegistry;

    public MusicSkillProductActivationProcessor(
            SkillProvider skillProvider,
            MegaMindRequestSkillApplier skillApplier,
            Phrases phrases,
            MetricRegistry metricRegistry,
            UserSkillProductConfig productConfig
    ) {
        this.skillProvider = skillProvider;
        this.skillApplier = skillApplier;
        this.phrases = phrases;
        this.metricRegistry = metricRegistry;
        this.skillProductConfig = productConfig;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return onInputClass(Input.Music.class)
                .and(IS_IN_SKILL)
                .and(isMusicActivation())
                .and(hasUserSkillProductFlag())
                .test(request);
    }

    private Predicate<MegaMindRequest<DialogovoState>> isMusicActivation() {
        return req -> req.getStateO().flatMap(DialogovoState::getProductActivationStateO)
                .map(ProductActivationState::getActivationType)
                .map(type -> type == ActivationType.MUSIC)
                .orElse(false);
    }

    @Override
    public RunRequestProcessorType getType() {
        return MUSIC_SKILL_PRODUCT_ACTIVATION;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        return new ApplyNeededResponse(
                RequestSkillApplyArguments.create(
                        getSkillId(request).get(),
                        Optional.empty(),
                        Optional.empty(),
                        Optional.empty())
        );
    }

    @Override
    public Class<RequestSkillApplyArguments> getApplyArgsType() {
        return RequestSkillApplyArguments.class;
    }

    @Override
    public ScenarioResponseBody<DialogovoState> apply(
            MegaMindRequest<DialogovoState> request,
            Context context,
            RequestSkillApplyArguments applyArguments
    ) {
        String skillId = getSkillId(request).get();
        int musicAttemptCount = request.getStateO().flatMap(DialogovoState::getProductActivationStateO)
                .map(ProductActivationState::getMusicAttemptCount)
                .orElse(MAX_MUSIC_ATTEMPT_COUNT);

        Input.Music musicInput = (Input.Music) request.getInput();

        if (needToRetryActivation(musicAttemptCount, musicInput)) {
            var state = request.getStateO()
                    .map(st -> st.withProductActivationState(
                            new ProductActivationState(musicAttemptCount + 1, ActivationType.MUSIC))
                    );

            context.getAnalytics().setIntent(MUSIC_SKILL_PRODUCT_ACTIVATION_RETRY);

            String retryText = phrases.getRandom(
                    musicInput.getMusicResult() == NOT_MUSIC ? RETRY_NOT_MUSIC_KEY : RETRY_DONT_MATCH_KEY,
                    request.getRandom()
            );

            metricRegistry.rate("skill.music.product.activation.event.rate",
                    Labels.of("skill_id", skillId, "result", "retry")).inc();
            logger.info("Retry of music activation. Skill id = " + skillId);
            return new ScenarioResponseBody<>(
                    Layout.builder()
                            .textCard(retryText)
                            .outputSpeech(retryText)
                            .shouldListen(false)
                            .directives(List.of(StartMusicRecognizerDirective.INSTANCE))
                            .build(),
                    state.orElse(null), // seems state is always non-null here
                    context.getAnalytics().toAnalyticsInfo(),
                    true,
                    Collections.emptyMap()
            );
        }
        return skillApplier.processApply(request, context, applyArguments);
    }

    private boolean needToRetryActivation(int musicAttemptCount, Input.Music musicInput) {
        var musicData = musicInput.getMusicData();
        boolean success = musicInput.getMusicResult() == SUCCESS;
        boolean matchAnyToken = skillProductConfig.getMusicIdToTokenCode().containsKey(musicData.getMusicId())
                || skillProductConfig.getMusicUrlToTokenCode().containsKey(musicData.getUrl());
        return !(matchAnyToken && success) && musicAttemptCount < MAX_MUSIC_ATTEMPT_COUNT;
    }

    private Predicate<MegaMindRequest<DialogovoState>> hasUserSkillProductFlag() {
        return request -> getSkillId(request).flatMap(skillProvider::getSkill)
                .map(skillInfo -> skillInfo.hasUserFeatureFlag(UserFeatureFlag.USER_SKILL_PRODUCT))
                .orElse(false);
    }
}
