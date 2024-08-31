package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.util.Currency;
import java.util.Set;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.services.promo.Platform;

@Data
class PromoInfoResponse {

    @JsonProperty("devices_with_multi_promo")
    private final Set<DeviceShortInfo> devicesWithMultiPromo;
    @JsonProperty("devices_with_personal_promo")
    private final Set<DeviceShortInfo> devicesWithPersonalPromo;

    @Data
    static class DeviceShortInfo {
        @JsonProperty("device_id")
        private final String deviceId;
        private final Platform platform;
        private final String name;
        @JsonProperty("promo_type")
        private final PromoType promo;

        @JsonProperty("promo_duration_days")
        public int getPromoDuration() {
            return promo.getDuration();
        }

        @JsonProperty("promo_payment_currency")
        public Currency getPromoPaymentCurrency() {
            return promo.getCurrency();
        }

        @JsonProperty("promo_payment_amount")
        public BigDecimal getPromoPaymentAmount() {
            return promo.getPrice();
        }
    }

}
