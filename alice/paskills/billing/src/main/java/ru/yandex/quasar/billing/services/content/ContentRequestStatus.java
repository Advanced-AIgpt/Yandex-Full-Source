package ru.yandex.quasar.billing.services.content;

import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.quasar.billing.providers.RejectionReason;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.services.OfferCardData;

public interface ContentRequestStatus {

    static ContentRequestStatus loginRequired(Set<String> providers) {
        return loginRequired(providers, null);
    }

    static ContentRequestStatus loginRequired(Set<String> providers, @Nullable OfferCardData offerCardData) {
        return new ProviderLoginRequested(providers, offerCardData);
    }

    static ContentRequestStatus available(Map<String, StreamData> providers) {
        return new ContentAvailable(providers);
    }

    static ContentRequestStatus paymentRequested(Map<String, RejectionReason> rejections) {
        return paymentRequested(rejections, null);
    }

    static ContentRequestStatus paymentRequested(
            Map<String, RejectionReason> rejections,
            @Nullable OfferCardData offerCardData
    ) {
        return new PurchaseRequested(false, rejections, offerCardData);
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    final class ContentAvailable implements ContentRequestStatus {
        private final Map<String, StreamData> providers;
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    final class ProviderLoginRequested implements ContentRequestStatus {
        private final Set<String> providers;
        @Nullable
        private final OfferCardData offerCardData;
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    final class PurchaseRequested implements ContentRequestStatus {
        private final boolean activeSubscriptionAvailable;
        private final Map<String, RejectionReason> providers;
        @Nullable
        private final OfferCardData offerCardData;
    }
}
