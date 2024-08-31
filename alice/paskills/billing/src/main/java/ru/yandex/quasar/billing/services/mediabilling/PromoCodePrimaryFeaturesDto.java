package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nullable;

record PromoCodePrimaryFeaturesDto(
        @Nullable PromoCodeDaysFeatureDto offerFreeDays,
        @Nullable PromoCodeSubscriptionFeatureDto subscription,
        @Nullable PromoCodePointsFeature points) {
}
