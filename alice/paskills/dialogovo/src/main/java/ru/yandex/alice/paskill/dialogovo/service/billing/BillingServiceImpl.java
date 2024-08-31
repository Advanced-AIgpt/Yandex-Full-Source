package ru.yandex.alice.paskill.dialogovo.service.billing;

import java.time.Duration;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferPaymentInfoResponse;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferStatusResponse;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationRequest;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductsResult;
import ru.yandex.passport.tvmauth.TvmClient;

import static java.util.Collections.emptyList;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.SERVICE_TICKET_HEADER;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.USER_TICKET_HEADER;

class BillingServiceImpl implements BillingService {
    private static final Logger logger = LogManager.getLogger();

    private final String billingUrl;
    private final Duration timeout;
    private final RestTemplate restTemplate;
    private final DialogovoInstrumentedExecutorService executor;
    private final TvmClient tvmClient;
    private final RequestContext requestContext;

    private static final UserSkillProductsResult EMPTY = new UserSkillProductsResult(emptyList());

    BillingServiceImpl(
            String billingUrl,
            Duration timeout,
            RestTemplate restTemplate,
            DialogovoInstrumentedExecutorService executor,
            TvmClient tvmClient,
            RequestContext requestContext
    ) {
        this.billingUrl = billingUrl;
        this.timeout = timeout;
        this.restTemplate = restTemplate;
        this.executor = executor;
        this.tvmClient = tvmClient;
        this.requestContext = requestContext;
    }

    @Override
    public CreatedPurchaseOffer createSkillPurchaseOffer(PurchaseOfferRequest request) throws BillingServiceException {
        logger.info("Start purchase offer request");

        try {
            var uri = UriComponentsBuilder.fromUriString(billingUrl)
                    .pathSegment("createSkillPurchaseOffer")
                    .build()
                    .toUri();

            logger.debug("Billing request uri: {} {}", uri, request);

            CreatedPurchaseOffer purchaseOffer = Objects.requireNonNull(
                    restTemplate.postForObject(
                            uri,
                            new HttpEntity<>(request, getBillingHeaders()),
                            CreatedPurchaseOffer.class
                    )
            );
            logger.debug("Retrieved purchase offer: {}", purchaseOffer);
            return purchaseOffer;
        } catch (HttpClientErrorException e) {
            var message = "Unable to request billing: request = " + request + ", responseBody = "
                    + e.getResponseBodyAsString();
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        } catch (Exception e) {
            String message = "Unable to request billing: request = " + request;
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        }
    }

    @Override
    public CompletableFuture<UserSkillProductsResult> getUserSkillProductsAsync(String skillId) {
        return executor.supplyAsyncInstrumented(() -> {
                    try {
                        return getUserSkillProducts(skillId);
                    } catch (BillingServiceException e) {
                        logger.error("Unable to obtain user skill products", e);
                    }
                    return EMPTY;
                },
                timeout,
                () -> EMPTY
        );
    }

    @Override
    public UserSkillProductsResult getUserSkillProducts(String skillId) throws BillingServiceException {
        logger.info("get skill products by user, skillId = {}", skillId);

        String uid = requestContext.getCurrentUserId();
        try {
            var uri = UriComponentsBuilder.fromUriString(billingUrl)
                    .pathSegment("user/skill_product", skillId)
                    .build()
                    .toUri();

            logger.debug("Billing request uri: {}", uri);

            UserSkillProductsResult userSkillProductsResult = restTemplate.exchange(
                    uri,
                    HttpMethod.GET,
                    new HttpEntity<>(getBillingHeaders()),
                    UserSkillProductsResult.class
            ).getBody();

            logger.debug(
                    "For skillId = {} and userId = {} billing user skill products result is: {}",
                    skillId,
                    uid,
                    userSkillProductsResult
            );

            return Objects.requireNonNull(userSkillProductsResult);
        } catch (HttpClientErrorException e) {
            var message = "Unable to request user skill products from billing: skillId = "
                    + skillId + ", userId = " + uid + "\n " + "responseBody =" + e.getResponseBodyAsString();
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        } catch (Exception e) {
            String message = "Exception during getting user skill products from billing: skillId = "
                    + skillId + ", userId = " + uid;
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        }
    }

    @Override
    public UserSkillProductActivationResult activateUserSkillProduct(SkillInfo skillInfo, String tokenCode)
            throws BillingServiceException {
        logger.info("start to activate user skill product");

        String uid = requestContext.getCurrentUserId();
        String skillId = skillInfo.getId();

        try {
            var uri = UriComponentsBuilder.fromUriString(billingUrl)
                    .pathSegment("user/skill_product", skillId, tokenCode)
                    .build()
                    .toUri();

            logger.debug("Billing request uri: {}", uri);

            var request = new UserSkillProductActivationRequest(skillInfo.getName(), skillInfo.getLogoUrl());
            var result = restTemplate.exchange(
                    uri,
                    HttpMethod.POST,
                    new HttpEntity<>(request, getBillingHeaders()),
                    UserSkillProductActivationResult.class
            ).getBody();

            logger.debug("Skill product activation for skillId = {} and userId = {} is: {}", skillId, uid, result);
            return Objects.requireNonNull(result);
        } catch (HttpClientErrorException e) {
            var message = "Unable to activate user skill product: skillId = "
                    + skillId + ", userId = " + uid + "\n " + "responseBody =" + e.getResponseBodyAsString();
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        } catch (Exception e) {
            String message = "Exception during activating of user skill product: skillId = "
                    + skillId + ", userId = " + uid;
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        }
    }

    @Override
    public PurchaseOfferStatusResponse getPurchaseStatus(String purchaseOfferUuid) throws BillingServiceException {
        logger.info("start to get purchase status");

        try {
            var uri = UriComponentsBuilder.fromUriString(billingUrl)
                    .pathSegment("/purchase_offer", purchaseOfferUuid, "/status")
                    .build()
                    .toUri();

            logger.debug("Billing request uri: {}", uri);

            var result = restTemplate.exchange(
                    uri,
                    HttpMethod.GET,
                    new HttpEntity<>(getBillingHeaders()),
                    PurchaseOfferStatusResponse.class
            ).getBody();

            logger.debug("Got purchase status. purchaseOfferUuid = {}. result = {}", purchaseOfferUuid, result);
            return Objects.requireNonNull(result);
        } catch (HttpClientErrorException e) {
            var message = "Unable to get purchase status: purchaseOfferUuid = " + purchaseOfferUuid + "\n "
                    + "responseBody =" + e.getResponseBodyAsString();
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        } catch (Exception e) {
            String message = "Exception getting purchase status: purchaseOfferUuid = " + purchaseOfferUuid;
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        }
    }

    @Override
    public PurchaseOfferPaymentInfoResponse getPurchaseOfferPaymentInfo(String purchaseOfferUuid)
            throws BillingServiceException {
        logger.info("start to get purchase payment info");

        try {
            var uri = UriComponentsBuilder.fromUriString(billingUrl)
                    .pathSegment("/purchase_offer", purchaseOfferUuid, "/payment")
                    .build()
                    .toUri();

            logger.debug("Billing request uri: {}", uri);

            var result = restTemplate.exchange(
                    uri,
                    HttpMethod.GET,
                    new HttpEntity<>(getBillingHeaders()),
                    PurchaseOfferPaymentInfoResponse.class
            ).getBody();

            logger.debug("Got purchase status. purchaseOfferUuid = {}. result = {}", purchaseOfferUuid, result);
            return Objects.requireNonNull(result);
        } catch (HttpClientErrorException e) {
            var message = "Unable to get purchase info: purchaseOfferUuid = " + purchaseOfferUuid + "\n "
                    + "responseBody =" + e.getResponseBodyAsString();
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        } catch (Exception e) {
            String message = "Exception getting purchase info: purchaseOfferUuid = " + purchaseOfferUuid;
            logger.error(message, e);
            throw new BillingServiceException(message, e);
        }
    }

    private HttpHeaders getBillingHeaders() {
        HttpHeaders headers = new HttpHeaders();
        headers.setContentType(MediaType.APPLICATION_JSON);
        headers.add(SERVICE_TICKET_HEADER, tvmClient.getServiceTicketFor("billing"));
        headers.add(USER_TICKET_HEADER, requestContext.getCurrentUserTicket());
        return headers;
    }
}
