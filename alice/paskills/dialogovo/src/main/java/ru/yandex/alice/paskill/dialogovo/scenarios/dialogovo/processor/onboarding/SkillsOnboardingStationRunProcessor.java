package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiPredicate;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SkillOnboardingAnalyticsInfoObject;

import static java.util.function.Predicate.not;

public class SkillsOnboardingStationRunProcessor implements RunRequestProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();

    private static final BiPredicate<MegaMindRequest, SkillsOnboardingDefinition> ONBOARDING_SELECTOR =
            (request, onboarding) -> request.hasAnySemanticFrame(onboarding.getSemanticFrames())
                    && (onboarding.getExperiment() == null || request.hasExperiment(onboarding.getExperiment()));

    private final Map<String, SkillsOnboardingDefinition> frameToOnboarding;

    public SkillsOnboardingStationRunProcessor(List<SkillsOnboardingDefinition> onboardings) {
        Map<String, SkillsOnboardingDefinition> frameOnboarding = new HashMap<>();
        for (SkillsOnboardingDefinition onboarding : onboardings) {
            for (String semanticFrame : onboarding.getSemanticFrames()) {
                frameOnboarding.put(semanticFrame, onboarding);
            }
        }
        this.frameToOnboarding = Map.copyOf(frameOnboarding);
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_SMART_SPEAKER_OR_TV
                .and(not(IS_IN_SKILL))
                .and(req -> frameToOnboarding
                        .values()
                        .stream()
                        .anyMatch(onboarding -> ONBOARDING_SELECTOR.test(req, onboarding)))
                .test(request);
    }

    @Override
    public ProcessorType getType() {
        return RunRequestProcessorType.SKILLS_ONBOARDING_STATION;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        // null safety provided by canProcess
        // refind matched semantic frame
        var onboarding = frameToOnboarding
                .values()
                .stream()
                .filter(onb -> ONBOARDING_SELECTOR.test(request, onb))
                .findFirst()
                .get();

        logger.info("Start with {} station skill onboarding", onboarding.getType());

        context.getAnalytics().addObject(new SkillOnboardingAnalyticsInfoObject(onboarding.getType()));
        return onboarding.getResponseGenerator().generateResponse(context, request);
    }
}
