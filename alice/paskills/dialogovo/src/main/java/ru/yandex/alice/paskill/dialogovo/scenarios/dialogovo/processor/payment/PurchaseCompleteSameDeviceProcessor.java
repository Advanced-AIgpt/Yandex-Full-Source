package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment;

import java.util.ArrayList;
import java.util.Map;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.server.DeletePusheDirective;
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.PurchaseCompleteAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.PurchaseCompleteApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.PurchaseCompleteDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.RequestSkillEvents;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingService;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingServiceException;
import ru.yandex.alice.paskill.dialogovo.service.purchase.PurchaseCompleteResponseDao;
import ru.yandex.alice.paskill.dialogovo.service.purchase.PurchaseCompleteResponseKey;
import ru.yandex.alice.paskills.common.billing.model.PaymentStatus;
import ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferPaymentInfoResponse;

@Component
public class PurchaseCompleteSameDeviceProcessor
        implements ApplyingRunProcessor<DialogovoState, PurchaseCompleteApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final RequestContext requestContext;
    private final PurchaseCompleteResponseDao purchaseCompleteResponseDao;
    private final BillingService billingService;
    private final Phrases phrases;
    private final SkillProvider skillProvider;
    private final MegaMindRequestSkillApplier skillApplier;

    public PurchaseCompleteSameDeviceProcessor(
            RequestContext requestContext,
            PurchaseCompleteResponseDao purchaseCompleteResponseDao,
            BillingService billingService,
            Phrases phrases,
            SkillProvider skillProvider,
            MegaMindRequestSkillApplier skillApplier
    ) {
        this.requestContext = requestContext;
        this.purchaseCompleteResponseDao = purchaseCompleteResponseDao;
        this.billingService = billingService;
        this.phrases = phrases;
        this.skillProvider = skillProvider;
        this.skillApplier = skillApplier;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return onCallbackDirective(PurchaseCompleteDirective.class)
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.PURCHASE_COMPLETE_SAME_DEVICE_CALLBACK;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        PurchaseCompleteDirective payload = convertPayload(request);

        logger.info("Start to process the purchase complete payload = {}", payload);

        String skillId = payload.getSkillId();
        String purchaseOfferUuid = payload.getPurchaseOfferUuid();

        var arguments = new PurchaseCompleteApplyArguments(skillId, purchaseOfferUuid, payload.getInitialDeviceId());

        return new ApplyNeededResponse(arguments);
    }

    @Override
    public ScenarioResponseBody<DialogovoState> apply(MegaMindRequest<DialogovoState> request, Context context,
                                                      PurchaseCompleteApplyArguments applyArguments) {

        logger.info("Start to process the purchase complete applyArguments = {}", applyArguments);

        String skillId = applyArguments.getSkillId();
        String userId = requestContext.getCurrentUserId();
        String purchaseOfferUuid = applyArguments.getPurchaseOfferUuid();

        var skill = skillProvider.getSkill(skillId)
                .orElseThrow(() -> new AliceHandledException("Skill not found " + skillId,
                        ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Навык не найден", "Навык не найден"));

        PurchaseOfferStatus purchaseOfferStatus;
        PaymentStatus paymentStatus;
        PurchaseOfferPaymentInfoResponse purchaseOfferPaymentInfo = null;
        if (skill.hasFeatureFlag(SkillFeatureFlag.PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK_SCHEME)) {
            // scheme with skill request on server_action
            try {
                purchaseOfferPaymentInfo = billingService.getPurchaseOfferPaymentInfo(purchaseOfferUuid);

                purchaseOfferStatus = purchaseOfferPaymentInfo.getPurchaseOfferStatus();
                paymentStatus = purchaseOfferPaymentInfo.getPaymentStatus();
            } catch (BillingServiceException e) {
                throw AliceHandledException.from(
                        "Billing didn't activate product. purchaseOfferUuid = " + purchaseOfferUuid,
                        ErrorAnalyticsInfoAction.PURCHASE_NOT_FOUND_FAILURE,
                        "Покупка не найдена",
                        true);
            }
        } else {
            // scheme with skill request on billing callback
            try {
                var purchaseStatus = billingService.getPurchaseStatus(purchaseOfferUuid);
                purchaseOfferStatus = purchaseStatus.getPurchaseOfferStatus();
                paymentStatus = purchaseStatus.getPaymentStatus();
            } catch (BillingServiceException e) {
                throw AliceHandledException.from(
                        "Billing didn't activate product. purchaseOfferUuid = " + purchaseOfferUuid,
                        ErrorAnalyticsInfoAction.PURCHASE_NOT_FOUND_FAILURE,
                        "Покупка не найдена",
                        true);
            }
        }

        logger.info("Purchase offer status: [{}]; purchase payment status: [{}]", purchaseOfferStatus, paymentStatus);

        Layout.Builder layoutB = Layout.builder()
                .shouldListen(true);

        Optional<DialogovoState> stateO = request.getStateO();
        var serverDirectives = new ArrayList<ServerDirective>();
        if (purchaseOfferStatus == PurchaseOfferStatus.SUCCESS || paymentStatus == PaymentStatus.AUTHORIZED) {
            // purchase success
            String defaultText = phrases.getRandom("purchase.complete.success_text.default", request.getRandom());

            context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_PURCHASE_COMPLETE);
            context.getAnalytics().addAction(PurchaseCompleteAction.INSTANCE);

            if (skill.hasFeatureFlag(SkillFeatureFlag.PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK_SCHEME)) {
                // scheme with skill request on server_action
                return skillApplier.processApply(
                        request,
                        context,
                        RequestSkillApplyArguments.create(
                                applyArguments.getSkillId(),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.empty()),
                        Optional.of(RequestSkillEvents.builder()
                                .purchaseCompleteEvent(Optional.of(
                                        new PurchaseCompleteEvent(
                                                purchaseOfferPaymentInfo.getPurchaseRequestId(),
                                                purchaseOfferPaymentInfo.getPurchaseOfferUuid(),
                                                purchaseOfferPaymentInfo.getPurchasePayload()
                                        )))
                                .build()));

            } else {
                // scheme with skill request on billing callback
                var key = new PurchaseCompleteResponseKey(userId, skillId, purchaseOfferUuid);
                Optional<WebhookResponse> responseO = purchaseCompleteResponseDao.findResponse(key);
                if (responseO.isPresent()) {
                    WebhookResponse response = responseO.get();
                    layoutB
                            .textCard(response.getResponse()
                                    .map(Response::getText)
                                    .orElse(defaultText))
                            .outputSpeech(response.getResponse()
                                    .map(Response::getTts)
                                    .orElse(defaultText));
                } else {
                    logger.warn("Purchase complete response not found with key = [{}] ", key);
                    layoutB
                            .textCard(defaultText)
                            .outputSpeech(defaultText);
                }
            }

            if (request.getClientInfo().isYaSmartDevice() && context.getSource().isByUser()) {
                serverDirectives.add(new DeletePusheDirective("alice-skill-start-purchase-request"));

                var purchaseSkillId = convertPayload(request).getSkillId();
                // if not in current skill now - start new skill session
                if (!IS_IN_SKILL
                        .and(req -> RunRequestProcessor.getSkillId(req)
                                .map(currSkillId -> currSkillId.equals(purchaseSkillId))
                                .orElse(false))
                        .test(request)) {
                    stateO = Optional.of(
                            DialogovoState.createSkillState(
                                    purchaseSkillId,
                                    Session.create(ActivationSourceType.PAYMENT, request.getServerTime()),
                                    0L,
                                    Optional.empty(),
                                    Optional.empty()
                            )
                    );
                }
            }
            // purchase failure
        } else {
            String defaultText = phrases.getRandom("purchase.complete.failure_text.default", request.getRandom());
            context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_PURCHASE_FAILED);

            layoutB
                    .textCard(defaultText)
                    .outputSpeech(defaultText);
        }

        var scenarioResponseBody = new ScenarioResponseBody<>(
                layoutB.build(),
                stateO,
                context.getAnalytics().toAnalyticsInfo(),
                true,
                Map.of(),
                serverDirectives
        );

        deleteResponse(userId, skillId, purchaseOfferUuid);

        return scenarioResponseBody;
    }

    public void deleteResponse(String userId, String skillId, String purchaseOfferUuid) {
        var key = new PurchaseCompleteResponseKey(userId, skillId, purchaseOfferUuid);

        logger.info("Deleting complete purchase response from ydb for key = {}", key);
        purchaseCompleteResponseDao.deleteResponse(key);
    }

    @Override
    public Class<PurchaseCompleteApplyArguments> getApplyArgsType() {
        return PurchaseCompleteApplyArguments.class;
    }

    private PurchaseCompleteDirective convertPayload(MegaMindRequest<DialogovoState> request) {
        return request.getInput().getDirective(PurchaseCompleteDirective.class);
    }
}
