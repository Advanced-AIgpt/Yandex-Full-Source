package ru.yandex.alice.paskill.dialogovo.scenarios;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor;

import static java.util.function.Predicate.not;

public abstract class BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor
        implements SingleSemanticFrameRunProcessor<DialogovoState> {

    private final String semanticFrameType;
    private final ProcessorType processorType;

    protected BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor(String semanticFrameType,
                                                                      ProcessorType processorType) {
        this.semanticFrameType = semanticFrameType;
        this.processorType = processorType;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return hasFrame()
                .and(not(IS_IN_SKILL))
                .and(req -> canProcessAdditionally(req, request.getSemanticFrameO(getSemanticFrame()).get()))
                .test(request);
    }

    @Override
    public final ProcessorType getType() {
        return processorType;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        return process(context, request, request.getSemanticFrameO(getSemanticFrame()).get());
    }

    @Override
    public final String getSemanticFrame() {
        return semanticFrameType;
    }

    protected abstract BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request,
                                               SemanticFrame semanticFrame);

    protected boolean canProcessAdditionally(MegaMindRequest<DialogovoState> request, SemanticFrame frame) {
        return true;
    }
}
