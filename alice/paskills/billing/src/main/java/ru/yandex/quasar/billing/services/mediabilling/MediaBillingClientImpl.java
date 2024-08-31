package ru.yandex.quasar.billing.services.mediabilling;

import java.net.URI;
import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.MusicApiConfig;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.promo.PromoInfo;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

@Component
public class MediaBillingClientImpl implements MediaBillingClient {

    private static final int MEDIABILLING_TRUST_SERVICE_ID = 711;
    private final RestTemplate restTemplate;
    private final MusicApiConfig musicApiConfig;
    private final TvmClient tvmClient;
    private final AuthorizationContext authorizationContext;
    private final ObjectMapper objectMapper;
    private final TypeReference<MediaBillingWrapper<ErrorResponseDto>> errorTypeRef = new TypeReference<>() {
    };

    private static final Logger logger = LogManager.getLogger();
    private static final String MUSIC_TVM_NAME = "music";
    private static final String MEDIABILLING_TVM_NAME = "mediabilling";

    public MediaBillingClientImpl(RestTemplate restTemplate,
                                  BillingConfig config,
                                  TvmClient tvmClient,
                                  AuthorizationContext authorizationContext,
                                  ObjectMapper objectMapper
    ) {
        this.restTemplate = restTemplate;
        this.musicApiConfig = config.getMusicApiConfig();
        this.tvmClient = tvmClient;
        this.authorizationContext = authorizationContext;
        this.objectMapper = objectMapper;
    }

    @Override
    public PromoCodeActivationResult activatePromoCode(long uid,
                                                       String code,
                                                       String paymentMethodId,
                                                       String origin,
                                                       @Nullable Integer region) {
        URI uri = UriComponentsBuilder.fromUriString(musicApiConfig.getMediabillingUrl())
                .pathSegment("promo-codes", "activate")
                .build().toUri();

        String serviceTicket = tvmClient.getServiceTicketFor(MEDIABILLING_TVM_NAME);

        // Music doesn't check TVM ticket now but is going to change it shortly
        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

        var request = new PromoCodeActivateRequestDto(code, uid, paymentMethodId,
                authorizationContext.getUserIp(), "web", "device", origin, region, List.of());

        try {
            var result = Objects.requireNonNull(restTemplate.exchange(uri, HttpMethod.POST,
                    new HttpEntity<>(request, headers),
                    new ParameterizedTypeReference<MediaBillingWrapper<PromoCodeActivateResponseDto>>() {
                    }
            ).getBody().getResult());

            logger.info("Promocode for user {} activated successfully with details: {}", uid, result);
            return new PromoCodeActivationResult(MusicPromoActivationResult.SUCCESS);

        } catch (HttpClientErrorException.BadRequest e) {
            var errorResponse = parseErrorResponse(e.getResponseBodyAsString());
            if (errorResponse == null) {
                throw new MediaBillingException(e);
            }

            logger.info("Promocode activation failed with error: {}", errorResponse);

            // mediabilling responds by enum name
            var status = MusicPromoActivationResult.byName(errorResponse.getName());
            return new PromoCodeActivationResult(status);
        } catch (HttpClientErrorException e) {
            if (e.getStatusCode() == HttpStatus.UNAVAILABLE_FOR_LEGAL_REASONS) {
                logger.info("Not allowed in current region due to 451 error with body {}", e.getResponseBodyAsString());
                return new PromoCodeActivationResult(MusicPromoActivationResult.CODE_NOT_ALLOWED_IN_CURRENT_REGION);
            }
            logger.error("Failed to activate promocode", e);
            throw refineException(e);
        }
    }

    @Override
    public PromoInfo prototypeFeatures(long uid,
                                       String prototype,
                                       @Nullable String platform,
                                       @Nullable Integer region) {

        URI uri = UriComponentsBuilder.fromUriString(musicApiConfig.getMediabillingUrl())
                .pathSegment("promo-codes", "features")
                .build().toUri();

        String serviceTicket = tvmClient.getServiceTicketFor(MEDIABILLING_TVM_NAME);

        // Music doesn't check TVM ticket now but is going to change it shortly
        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

        var request =
                new PromoCodeFeaturesRequestDto(uid, prototype, authorizationContext.getUserIp(), platform, region);

        try {
            var result = Objects.requireNonNull(restTemplate.exchange(uri, HttpMethod.POST, new HttpEntity<>(request,
                            headers),
                    new ParameterizedTypeReference<MediaBillingWrapper<PromoCodeFeaturesDto>>() {
                    }).getBody());

            return toPromoInfo(Objects.requireNonNull(result.getResult()).primaryFeatures());

        } catch (HttpClientErrorException.BadRequest e) {
            var errorResponse = parseErrorResponse(e.getResponseBodyAsString());
            if (errorResponse == null) {
                throw new MediaBillingException(e);
            }

            logger.info("Obtaining Promocode features failed with error: {}", errorResponse);

            // mediabilling responds by enum name
            var status = MusicPromoActivationResult.byName(errorResponse.getName());


            logger.info("Promocode can't be activated for user {} region {} with error: {}", uid, region, status);
            throw new PromocodeException(status);

        } catch (HttpClientErrorException e) {
            logger.error("Failed to activate promocode", e);
            throw refineException(e);
        }

    }

    private PromoInfo toPromoInfo(PromoCodePrimaryFeaturesDto primaryFeatures) {
        PromoInfo promoInfo;
        if (primaryFeatures.offerFreeDays() != null) {
            PromoCodeDaysFeatureDto offerFreeDays = primaryFeatures.offerFreeDays();
            promoInfo = new PromoInfo.FreeDays(offerFreeDays.offer().id(), offerFreeDays.amount());
        } else if (primaryFeatures.subscription() != null) {
            PromoCodeSubscriptionFeatureDto.Invoice invoice = primaryFeatures.subscription().firstInvoice();

            String productId = primaryFeatures.subscription().offer().id();
            if (invoice != null) {
                Integer promoDays = Math.toIntExact(Duration.between(Instant.now(), invoice.timestamp()).toDays());

                promoInfo = new PromoInfo.Subscription(productId,
                        promoDays,
                        invoice.totalPrice().amount(),
                        invoice.totalPrice().currency(),
                        invoice.timestamp());
            } else {
                promoInfo = new PromoInfo.SubscriptionPartly(productId);
            }
        } else if (primaryFeatures.points() != null) {
            promoInfo = new PromoInfo.PlusPoints(primaryFeatures.points().amount());
        } else {
            logger.error("Unexpected prototype features: {}", primaryFeatures);
            throw new MediaBillingException("Unexpected prototype features");
        }
        return promoInfo;

    }

    @Nullable
    ErrorResponseDto parseErrorResponse(String body) {
        try {
            return objectMapper.readValue(body, errorTypeRef).getResult();
        } catch (JsonProcessingException e) {
            logger.error("Failed to parse mediabilling ErrorResponseDto", e);
            return null;
        }
    }

    @Override
    public SubmitNativeOrderResult submitNativeOrder(String uid, String productId, String paymentCardId) {
        URI uri = UriComponentsBuilder.fromUriString(musicApiConfig.getApiUrl())
                .pathSegment("internal-api", "account", "submit-native-order")
                .queryParam("__uid", uid)
                .queryParam("ip", authorizationContext.getUserIp())
                .queryParam("productId", productId)
                .queryParam("paymentMethodId", paymentCardId)
                .queryParam("activateImmediately", true)
                .build()
                .toUri();

        String serviceTicket = tvmClient.getServiceTicketFor(MUSIC_TVM_NAME);

        // Music doesn't check TVM ticket now but is going to change it shortly
        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

        try {
            SubmitNativeOrderResult result = restTemplate.postForObject(uri, new HttpEntity<>(headers),
                            SubmitNativeOrderResult.Wrapper.class)
                    .getResult();
            return result;
        } catch (HttpClientErrorException e) {
            throw refineException(e);
        }
    }

    @Override
    public String clonePrototype(String prototype) {
        URI uri = UriComponentsBuilder.fromUriString(musicApiConfig.getMediabillingUrl())
                .pathSegment("billing", "promo-code", "clone")
                .queryParam("prototype", prototype)
                .build().toUri();
        String serviceTicket = tvmClient.getServiceTicketFor(MEDIABILLING_TVM_NAME);

        // Music doesn't check TVM ticket now but is going to change it shortly
        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

        try {
            PromoCodeCloneResult result = restTemplate.postForObject(uri, new HttpEntity<>(headers),
                            PromoCodeCloneResult.Wrapper.class)
                    .getResult();
            return result.getCode();
        } catch (HttpClientErrorException e) {
            logger.error("failed to clone promocode", e);
            throw refineException(e);
        }

    }

    @Override
    public OrderInfo getOrderInfo(String uid, String orderId) {
        URI uri = UriComponentsBuilder.fromUriString(musicApiConfig.getApiUrl())
                .pathSegment("internal-api", "account", "order-info")
                .queryParam("__uid", uid)
                .queryParam("orderId", orderId)
                .build()
                .toUri();

        String serviceTicket = tvmClient.getServiceTicketFor(MUSIC_TVM_NAME);

        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

        try {
            var response = restTemplate.exchange(uri,
                    HttpMethod.GET,
                    new HttpEntity<>(headers),
                    OrderInfo.Wrapper.class);

            return response.getBody().getResult();
        } catch (HttpClientErrorException e) {
            throw refineException(e);
        }
    }

    @Override
    public BindCardResult bindCard(String uid, String userIp, String backUrl, TemplateTag template) {
        URI uri = UriComponentsBuilder.fromUriString(musicApiConfig.getMediabillingUrl())
                .pathSegment("payment", "bind-card")
                .queryParam("__uid", uid)
                .queryParam("ip", userIp)
                .queryParam("templateTag", template.getTag())
                // let trust automatically determine currency
                //.queryParam("currency", currency.getCurrencyCode())
                .queryParam("returnPath", backUrl)
                .queryParam("trustService", MEDIABILLING_TRUST_SERVICE_ID)

                .build().toUri();
        String serviceTicket = tvmClient.getServiceTicketFor(MEDIABILLING_TVM_NAME);

        // Music doesn't check TVM ticket now but is going to change it shortly
        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

        try {
            BindCardResult result = restTemplate.postForObject(uri, new HttpEntity<>(headers),
                            BindCardResult.Wrapper.class)
                    .result();
            return result;
        } catch (HttpClientErrorException e) {
            logger.error("failed to bind card", e);
            throw refineException(e);
        }
    }

    private RuntimeException refineException(HttpClientErrorException e) {
        if (e.getStatusCode() == HttpStatus.UNAUTHORIZED || e.getStatusCode() == HttpStatus.FORBIDDEN) {
            return new UnauthorizedException("MediaBilling exception unauthorized", e);
        } else if (e.getStatusCode() == HttpStatus.NOT_FOUND) {
            return new NotFoundException("MediaBilling not found", e);
        } else {
            return new MediaBillingException(e);
        }
    }


}
