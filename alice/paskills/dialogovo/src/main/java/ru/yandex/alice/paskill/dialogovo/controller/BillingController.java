package ru.yandex.alice.paskill.dialogovo.controller;

import java.time.Instant;
import java.util.Optional;
import java.util.Random;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessorImpl;
import ru.yandex.alice.paskill.dialogovo.providers.EventIdProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.service.purchase.PurchaseCompleteResponseDao;
import ru.yandex.alice.paskill.dialogovo.service.purchase.PurchaseCompleteResponseKey;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@RestController
class BillingController {
    private static final Logger logger = LogManager.getLogger();

    private final SkillRequestProcessor skillRequestProcessor;
    private final SkillProvider skillProvider;
    private final PurchaseCompleteResponseDao purchaseCompleteResponseDao;
    private final EventIdProvider eventIdProvider;
    private final Rate billingCallbackFailureCounter;
    private final Rate skillDenyPurchaseCounter;
    private final RequestContext requestContext;

    BillingController(
            SkillRequestProcessorImpl skillRequestProcessor,
            SkillProvider skillProvider,
            MetricRegistry metricRegistry,
            PurchaseCompleteResponseDao purchaseCompleteResponseDao,
            EventIdProvider eventIdProvider,
            RequestContext requestContext) {
        this.skillRequestProcessor = skillRequestProcessor;
        this.skillProvider = skillProvider;
        this.purchaseCompleteResponseDao = purchaseCompleteResponseDao;
        this.eventIdProvider = eventIdProvider;
        this.billingCallbackFailureCounter = metricRegistry.rate(
                "business_failure",
                Labels.of(
                        "aspect", "billing_controller",
                        "process", "purchase_callback",
                        "reason", "unhandled_alice_exception"
                )
        );
        this.skillDenyPurchaseCounter = metricRegistry.rate(
                "business_failure",
                Labels.of(
                        "aspect", "billing_controller",
                        "process", "purchase_callback",
                        "reason", "skill_deny_purchase"
                )
        );
        this.requestContext = requestContext;
    }

    @PostMapping(value = "billing/skill-purchase/callback")
    public SkillPurchaseCallbackResponse skillPurchaseCallback(@RequestBody SkillPurchaseCallbackRequest request) {
        var now = Instant.now();
        var userId = request.getUserId();
        var skillId = request.getSkillId();
        var purchaseOfferUuid = request.getPurchaseOfferUuid();
        var key = new PurchaseCompleteResponseKey(userId, skillId, purchaseOfferUuid);

        logger.info("Start purchase callback for key = {}", key);
        var skill = skillProvider.getSkill(skillId)
                .orElseThrow(() -> AliceHandledException.from(
                        "Skill not found " + skillId,
                        ErrorAnalyticsInfoAction.CALLBACK_PURCHASE_FAILURE,
                        "Навык не найден",
                        true));

        ActivationSourceType sourceType = ActivationSourceType.UNDETECTED;

        PurchaseWebhookRequest webhookRequest = request.getPurchaseWebhookRequest();
        var newSession = request.getPurchaseWebhookRequest().convertToSession(eventIdProvider.generateEventId());

        SkillProcessRequest skillProcessRequest = SkillProcessRequest.builder()
                .skill(skill)
                .originalUtterance("")
                .normalizedUtterance("")
                .skillPurchaseConfirmEvent(request)
                .locationInfo(webhookRequest.convertToLocationInfo())
                .experiments(webhookRequest.getExperiments())
                .random(new Random())
                .clientInfo(webhookRequest.convertToClientInfo())
                .viewState(Optional.empty())
                .session(newSession)
                .activationSourceType(sourceType)
                .requestTime(now)
                .mementoData(RequestProto.TMementoData.getDefaultInstance())
                .build();

        Context context = new Context(SourceType.USER);
        requestContext.setCurrentUserId(userId);
        try {
            var result = skillRequestProcessor.process(context, skillProcessRequest);

            logger.info("End purchase callback for key = {}. Result = {}", key, result);

            boolean isSkillConfirmPurchaseComplete = result.getResponseO()
                    .flatMap(WebhookRequestResult::getResponse)
                    .flatMap(WebhookResponse::getResponse)
                    .flatMap(Response::getDirectives)
                    .flatMap(Directives::getConfirmPurchase)
                    .isPresent();

            @Nullable TextCard joinedTextCard = result.getLayout().joinTextCards();
            @Nullable String joinedTextCardText = joinedTextCard != null ? joinedTextCard.getText() : null;
            String resultText = Optional.ofNullable(joinedTextCardText).orElse("");
            if (isSkillConfirmPurchaseComplete) {
                if (!skill.hasFeatureFlag(SkillFeatureFlag.PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK_SCHEME)) {
                    // scheme with skill request on billing callback
                    result.getResponseO()
                            .flatMap(WebhookRequestResult::getResponse)
                            .ifPresent(response -> purchaseCompleteResponseDao.storeResponse(key, now, response));
                }
                return new SkillPurchaseCallbackResponse(CallbackResultStatus.OK, resultText);
            } else {
                skillDenyPurchaseCounter.inc();
                return new SkillPurchaseCallbackResponse(CallbackResultStatus.ERROR, resultText);
            }
        } catch (Exception e) {
            billingCallbackFailureCounter.inc();
            String message = "Error during skill request processing";
            logger.error(message, e);
            return new SkillPurchaseCallbackResponse(CallbackResultStatus.ERROR, message);
        }
    }


    public record SkillPurchaseCallbackResponse(
            @JsonProperty("result") CallbackResultStatus result,
            @JsonProperty("result_message") String resultMessage
    ) {
    }

    public enum CallbackResultStatus {
        STATUS_NOT_DEFINED("status_not_defined"),
        OK("ok"),
        ERROR("error");

        @Getter
        @JsonValue
        private final String value;

        CallbackResultStatus(String value) {
            this.value = value;
        }
    }
}
