package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.PurchaseCompleteApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.PurchaseCompleteDirective;
import ru.yandex.alice.paskill.dialogovo.service.xiva.XivaService;

import static ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor.getSkillId;
import static ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType.PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK;

@Component
public class PurchaseCompleteReturnToStationProcessor
        implements CommittingRunProcessor<DialogovoState, PurchaseCompleteApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final Phrases phrases;
    private final XivaService xivaService;
    private final RequestContext requestContext;

    public PurchaseCompleteReturnToStationProcessor(
            Phrases phrases,
            XivaService xivaService,
            RequestContext requestContext
    ) {
        this.phrases = phrases;
        this.xivaService = xivaService;
        this.requestContext = requestContext;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return onCallbackDirective(PurchaseCompleteDirective.class)
                .and(IS_IN_SKILL)
                .and(req -> getSkillId(req)
                        .map(r -> r.equals(convertToPurchaseCompleteDirective(req).getSkillId()))
                        .orElse(false))
                .and(req -> {
                    String currentDeviceId = req.getClientInfo().getDeviceIdO().orElse("");
                    return !currentDeviceId.equals(convertToPurchaseCompleteDirective(req).getInitialDeviceId());
                })
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        PurchaseCompleteDirective payload = convertToPurchaseCompleteDirective(request);

        var arguments = new PurchaseCompleteApplyArguments(
                payload.getSkillId(),
                payload.getPurchaseOfferUuid(),
                payload.getInitialDeviceId()
        );

        String text = phrases.getRandom("purchase.complete.return_to_initial_device", request.getRandom());

        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_RETURN_TO_STATION_AFTER_PURCHASE);

        var scenarioResponseBody = new ScenarioResponseBody<>(
                Layout.builder()
                        .textCard(text)
                        .shouldListen(false)
                        .build(),
                request.getStateO(),
                context.getAnalytics().toAnalyticsInfo(),
                false
        );

        return new CommitNeededResponse<>(scenarioResponseBody, arguments);
    }

    @Override
    public Class<PurchaseCompleteApplyArguments> getApplyArgsType() {
        return PurchaseCompleteApplyArguments.class;
    }

    @Override
    public CommitResult commit(
            MegaMindRequest<DialogovoState> request,
            Context context,
            PurchaseCompleteApplyArguments applyArguments
    ) {
        var directive = convertToPurchaseCompleteDirective(request);
        String initialDeviceId = applyArguments.getInitialDeviceId();

        if (requestContext.getCurrentUserId() != null) {
            xivaService.sendCallbackDirectiveAsync(requestContext.getCurrentUserId(), initialDeviceId, directive);
        } else {
            logger.warn(
                    "Trying to send xiva push to complete purchase but no user_id provided. device_id = {}",
                    initialDeviceId
            );
        }

        return CommitResult.Success;
    }

    private PurchaseCompleteDirective convertToPurchaseCompleteDirective(MegaMindRequest<DialogovoState> request) {
        return request.getInput().getDirective(PurchaseCompleteDirective.class);
    }
}
