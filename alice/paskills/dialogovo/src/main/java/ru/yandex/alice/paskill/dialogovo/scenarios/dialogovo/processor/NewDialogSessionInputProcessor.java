package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.NewSessionDirective;

@Component
public class NewDialogSessionInputProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {
    private final ProcessRunSkillRequestService processRunSkillRequestService;
    private final MegaMindRequestSkillApplier skillApplier;

    protected NewDialogSessionInputProcessor(
            ProcessRunSkillRequestService processRunSkillRequestService,
            MegaMindRequestSkillApplier skillApplier
    ) {
        this.processRunSkillRequestService = processRunSkillRequestService;
        this.skillApplier = skillApplier;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.NEW_DIALOG_SESSION;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return request.getInput().isCallback(NewSessionDirective.class);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        NewSessionDirective payload = request.getInput().getDirective(NewSessionDirective.class);

        Optional<ActivationSourceType> sourceType = payload.getSource()
                .flatMap(ActivationSourceType.R::fromValueO);

        return processRunSkillRequestService.activateSkill(
                payload.getDialogId(),
                new SkillActivationArguments(
                        context,
                        request,
                        sourceType,
                        payload.getRequest(),
                        payload.getOriginalUtterance(),
                        payload.getPayload(),
                        payload.getActivationTypedSemanticFrame()
                )
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
