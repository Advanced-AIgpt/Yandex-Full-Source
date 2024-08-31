package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.domain.QuasarAuxiliaryConfig;
import ru.yandex.alice.kronstadt.core.input.UtteranceInput;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.service.MatchedActivationIntentFinder;

import static java.util.function.Predicate.not;
import static ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType.SKILL_INTENT;
import static ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType.EXTERNAL_SKILL_INTENT_ACTIVATE;

@Component
public class IntentActivateRunProcessor implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {

    private final ProcessRunSkillRequestService processRunSkillRequestService;
    private final MatchedActivationIntentFinder matchedActivationIntentFinder;
    private final MegaMindRequestSkillApplier skillApplier;

    public IntentActivateRunProcessor(
            ProcessRunSkillRequestService processRunSkillRequestService,
            MatchedActivationIntentFinder matchedActivationIntentFinder,
            MegaMindRequestSkillApplier skillApplier) {
        this.processRunSkillRequestService = processRunSkillRequestService;
        this.matchedActivationIntentFinder = matchedActivationIntentFinder;
        this.skillApplier = skillApplier;
    }

    @Override
    public RunRequestProcessorType getType() {
        return EXTERNAL_SKILL_INTENT_ACTIVATE;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return not(IS_IN_SKILL)
                .and(req -> req.getInput() instanceof UtteranceInput)
                .and(req -> req.getOptions()
                        .getQuasarAuxiliaryConfig()
                        .getAlice4BusinessO()
                        .map(QuasarAuxiliaryConfig.Alice4BusinessConfig::getPreactivatedSkillIds).orElse(List.of())
                        .size() > 0)
                .test(request);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        var input = (UtteranceInput) request.getInput();

        List<String> skillIds = request.getOptions()
                .getQuasarAuxiliaryConfig()
                .getAlice4BusinessO()
                .map(QuasarAuxiliaryConfig.Alice4BusinessConfig::getPreactivatedSkillIds).orElse(List.of());

        Map<String, Set<String>> matchedSkills = matchedActivationIntentFinder.find(skillIds,
                input.getOriginalUtterance());

        if (!matchedSkills.isEmpty()) {
            return processRunSkillRequestService.activateSkill(
                    // activate first matched
                    matchedSkills
                            .entrySet()
                            .stream()
                            .findFirst()
                            .get()
                            .getKey(),
                    new SkillActivationArguments(
                            context,
                            request,
                            Optional.of(SKILL_INTENT),
                            input.getNormalizedUtterance(),
                            input.getOriginalUtterance(),
                            Optional.empty(),
                            null
                    )
            );
        }

        return DefaultIrrelevantResponse.create(
                Intents.ALICE4BUSINESS_DEVICE_LOCK_IRRELEVANT,
                request.getRandom(),
                false
        );
    }

    @Override
    public Class<RequestSkillApplyArguments> getApplyArgsType() {
        return RequestSkillApplyArguments.class;
    }

    @Override
    public ScenarioResponseBody<DialogovoState> apply(MegaMindRequest<DialogovoState> request, Context context,
                                                      RequestSkillApplyArguments applyArguments) {
        return skillApplier.processApply(request, context, applyArguments);
    }
}
