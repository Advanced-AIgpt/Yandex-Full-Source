package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;

import static ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor.getSkillId;

@Component
public class ProcessSkillRunProcessor implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {

    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final MegaMindRequestSkillApplier skillApplier;

    protected ProcessSkillRunProcessor(SkillProvider skillProvider, MegaMindRequestSkillApplier skillApplier) {
        this.skillProvider = skillProvider;
        this.skillApplier = skillApplier;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.PROCESS_SKILL;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_IN_SKILL.test(request);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        var skillInfo = getSkillId(request)
                .flatMap(skillProvider::getSkill)
                .orElseThrow(() -> new AliceHandledException(
                        "Навык не найден", ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Произошла ошибка"));

        context.getAnalytics().addObject(new SkillAnalyticsInfoObject(skillInfo));

        request.getDialogIdO().ifPresent(__ -> logger.info("inside skill tab"));

        Optional<ActivationSourceType> activationSourceType = request.getStateO()
                .flatMap(DialogovoState::getSessionO)
                .map(Session::getActivationSourceType)
                .filter(source -> source != ActivationSourceType.UNDETECTED);
        return new ApplyNeededResponse(RequestSkillApplyArguments.create(
                skillInfo.getId(),
                Optional.empty(),
                Optional.empty(),
                activationSourceType));
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
