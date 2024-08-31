package ru.yandex.alice.paskill.dialogovo.processor;

import java.net.URI;
import java.util.List;
import java.util.Objects;
import java.util.Random;
import java.util.UUID;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.domain.StartPurchaseButton;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.StartPurchase;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingService;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingServiceException;
import ru.yandex.alice.paskill.dialogovo.service.billing.CreatedPurchaseOffer;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseOfferRequest;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseOfferRequest.BillingSkillInfo;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseWebhookRequest.PurchaseRequestClientInfo;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseWebhookRequest.PurchaseRequestLocationInfo;
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseWebhookRequest.PurchaseRequestSession;

@Component
class StartPurchaseResponseHandler implements WebhookResponseHandler {
    private static final Logger logger = LogManager.getLogger();

    private final BillingService billingService;
    private final DialogovoDirectiveFactory directiveFactory;
    private final RequestContext requestContext;
    private final Phrases phrases;
    private final String dialogovoUrl;

    StartPurchaseResponseHandler(
            @Value("${dialogovoUrl}") String dialogovoUrl,
            BillingService billingService,
            DialogovoDirectiveFactory directiveFactory,
            RequestContext requestContext,
            Phrases phrases
    ) {
        this.dialogovoUrl = dialogovoUrl;
        this.billingService = billingService;
        this.directiveFactory = directiveFactory;
        this.requestContext = requestContext;
        this.phrases = phrases;
    }

    @Override
    public SkillProcessResult.Builder handleResponse(
            SkillProcessResult.Builder builder,
            SkillProcessRequest request,
            Context context,
            RequestEnrichmentData requestEnrichment,
            WebhookResponse response
    ) {
        var skillInfo = request.getSkill();
        var clientInfo = request.getClientInfo();

        checkClientSupportPurchase(clientInfo, request.getRandom());
        checkCryptoKeys(skillInfo, request.getRandom());

        builder.getLayout().shouldListen(false);

        PurchaseOfferRequest billingRequest = new PurchaseOfferRequest(
                BillingSkillInfo.from(skillInfo, getCallbackUrl()),
                request.getSession().map(Session::getSessionId).orElse(UUID.randomUUID().toString()),
                clientInfo.getUuid(),
                clientInfo.getDeviceIdO().orElse(""),
                getStartPurchase(response, request.getRandom()),
                new PurchaseWebhookRequest(
                        PurchaseRequestSession.from(request.getSession()),
                        PurchaseRequestLocationInfo.from(request.getLocationInfo()),
                        request.getExperiments(),
                        PurchaseRequestClientInfo.from(request.getClientInfo())
                )
        );
        CreatedPurchaseOffer billingResponse;
        try {
            if (context.getSource().isByUser()) {
                billingResponse = billingService.createSkillPurchaseOffer(billingRequest);
            } else {
                // don't create purchase offers on pings
                billingResponse = new CreatedPurchaseOffer(
                        "dummy-" + UUID.randomUUID().toString(),
                        URI.create("http://dummy_url")
                );
            }
        } catch (BillingServiceException e) {
            throw AliceHandledException.from(
                    "Billing response body was empty upon purchase offer",
                    ErrorAnalyticsInfoAction.START_PURCHASE_FAILURE,
                    phrases.getRandom("purchase.start.handle_directive.failed", request.getRandom()),
                    true);
        }

        if (clientInfo.isYaSmartDevice()) {
            if (requestContext.getCurrentUserId() != null && context.getSource().isByUser()) {
                // currentUserId is always present for station requests but may be absent for dev-console
                logger.info("send start purchase push for user {}", requestContext.getCurrentUserId());
                builder.serverDirective(new SendPushMessageDirective(
                        "Навык: " + skillInfo.getName(),
                        "Запрос на оплату в навыке",
                        billingResponse.getUrl(),
                        "alice_skill_start_purchase",
                        "alice-skill-start-purchase-request",
                        SendPushMessageDirective.ALICE_DEFAULT_DEVICE_ID,
                        List.of(SendPushMessageDirective.AppType.SEARCH_APP),
                        "Оплата в навыке " + skillInfo.getName(),
                        "Оплатить"
                ));
            } else {
                logger.warn(
                        "Trying to send sup push for purchase but no user_id provided. device_id = {}",
                        clientInfo.getDeviceIdO().orElse("-")
                );
            }

            var text = phrases.getRandom("purchase.start.require_push.answer", request.getRandom());
            builder.getLayout().textCard(new TextCard(text));
            return builder
                    .appendTts(text, Voice.SHITOVA_US, " ");
        } else {
            var text = phrases.getRandom("purchase.start.dont_require_push.answer", request.getRandom());
            URI offerUrl = billingResponse.getUrl();
            var offerUuid = billingResponse.getOrderId();
            var directive = directiveFactory.createLinkDirective(offerUrl);
            TextCard textCard = TextCard.builder()
                    .setText(text)
                    .button(new StartPurchaseButton("Оплатить", directive, offerUrl, offerUuid))
                    .build();
            builder.getLayout().textCard(textCard);
            return builder
                    .appendTts(text, Voice.SHITOVA_US, " ");
        }
    }

    private String getCallbackUrl() {
        return UriComponentsBuilder.fromUriString(dialogovoUrl)
                .pathSegment("billing/skill-purchase/callback")
                .build()
                .toUriString();
    }

    private StartPurchase getStartPurchase(WebhookResponse response, Random random) {
        return response.getResponse()
                .flatMap(Response::getDirectives)
                .flatMap(Directives::getStartPurchase)
                .orElseThrow(() -> AliceHandledException.from(
                        "Unable to handle start purchase directive",
                        ErrorAnalyticsInfoAction.START_PURCHASE_FAILURE,
                        phrases.getRandom("purchase.start.handle_directive.failed", random),
                        true));
    }

    private void checkClientSupportPurchase(ClientInfo clientInfo, Random random) {
        if (!clientInfo.isSupportPurchase()) {
            throw AliceHandledException.from(
                    "Surface is not supported",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                    phrases.getRandom("purchase.cant_start.unsupported_device", random),
                    true);
        }
    }

    private void checkCryptoKeys(SkillInfo skillInfo, Random random) {
        if (Objects.isNull(skillInfo.getPrivateKey()) || Objects.isNull(skillInfo.getPublicKey())) {
            throw AliceHandledException.from(
                    "Skill purchase doesn't setup, for skillId = " + skillInfo.getId() +
                            "It doesn't have private and/or public key.",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                    phrases.getRandom("purchase.cant_start.skill_didnt_setup_purchases", random),
                    true);
        }
    }
}
