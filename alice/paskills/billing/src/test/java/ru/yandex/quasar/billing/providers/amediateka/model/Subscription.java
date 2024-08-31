package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class Subscription {

    private final boolean status;
    @JsonProperty("bundle_uid")
    private final String bundleUid;
    @JsonProperty("subscription_period")
    private final Integer subscriptionPeriod;

    @Data
    public static class SubscriptionsDTO {

        @Nullable
        private final List<Subscription> subscriptions;

        @Nullable
        private final Subscription subscription;
    }

}
