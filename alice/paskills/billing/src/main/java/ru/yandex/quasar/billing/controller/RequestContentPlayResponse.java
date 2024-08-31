package ru.yandex.quasar.billing.controller;

import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.quasar.billing.providers.RejectionReason;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.services.OfferCardData;

import static java.util.stream.Collectors.toMap;
import static ru.yandex.quasar.billing.controller.PersonalCard.convertToPersonalCard;

@Data
@AllArgsConstructor(access = AccessLevel.PACKAGE)
@JsonInclude(JsonInclude.Include.NON_NULL)
class RequestContentPlayResponse {
    private static final Object DUMMY = new Object();
    private final RequestContentPlayStatus status;
    @JsonProperty("providers_to_login")
    @Nullable
    private final Set<String> providersToLogin;
    @Nullable
    private final Map<String, Object> providers;
    @Nullable
    @JsonProperty("any_active_subscription_exists")
    private final Boolean anyActiveSubscription;
    @Nullable
    @JsonProperty("personal_card")
    private final PersonalCard personalCard;

    @Nullable
    @JsonProperty("provider_rejection_reasons")
    private final Map<String, RejectionReason> providersRejectionReasons;

    static RequestContentPlayResponse loginRequiredResponse(Set<String> providers) {
        return loginRequiredResponse(providers, null);
    }

    static RequestContentPlayResponse loginRequiredResponse(
            Set<String> providers,
            @Nullable OfferCardData offerCardData
    ) {
        return new RequestContentPlayResponse(RequestContentPlayStatus.PROVIDER_LOGIN_REQUIRED, providers,
                null, null, convertToPersonalCard(offerCardData), null);
    }

    static RequestContentPlayResponse availableResponse(Map<String, StreamData> providers) {
        return new RequestContentPlayResponse(
                RequestContentPlayStatus.AVAILABLE,
                null,
                providers.entrySet()
                        .stream()
                        .collect(toMap(Map.Entry::getKey,
                                entry -> StreamData.EMPTY.equals(entry.getValue()) ?
                                        DummyEmpty.INSTANCE :
                                        entry.getValue())),
                null,
                null,
                null
        );
    }

    static RequestContentPlayResponse paymentRequestedResponse(
            boolean activeSubscriptionExists,
            Map<String, RejectionReason> providersRejectionReasons
    ) {
        return paymentRequestedResponse(activeSubscriptionExists, providersRejectionReasons, null);
    }
    static RequestContentPlayResponse paymentRequestedResponse(
            boolean activeSubscriptionExists,
            Map<String, RejectionReason> providersRejectionReasons,
            @Nullable OfferCardData offerCardData
    ) {
        return new RequestContentPlayResponse(RequestContentPlayStatus.PAYMENT_REQUIRED, null, null,
                activeSubscriptionExists, convertToPersonalCard(offerCardData), providersRejectionReasons);
    }

    enum RequestContentPlayStatus {
        AVAILABLE("available"),
        PAYMENT_REQUIRED("payment_required"),
        PROVIDER_LOGIN_REQUIRED("provider_login_required");

        private final String code;


        RequestContentPlayStatus(String code) {
            this.code = code;
        }

        @JsonValue
        public String getCode() {
            return code;
        }
    }

    @JsonInclude(JsonInclude.Include.NON_NULL)
    static class DummyEmpty {
        static final DummyEmpty INSTANCE = new DummyEmpty();
        private String dummy = null;
    }


}
