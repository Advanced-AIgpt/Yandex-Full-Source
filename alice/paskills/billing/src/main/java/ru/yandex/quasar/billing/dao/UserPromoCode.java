package ru.yandex.quasar.billing.dao;

import java.time.LocalDateTime;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import lombok.ToString;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;

@Getter
@Setter
@ToString
@NoArgsConstructor
public class UserPromoCode {
    private Long uid;
    private String code;
    @Nullable // for backward compatibility
    private String provider;
    private PricingOptionType pricingOptionType;
    @Nullable
    private Integer subscriptionPeriod;
    @Nullable
    private PricingOption subscriptionPricingOption;

    private LocalDateTime activatedAt;

    @Nullable
    private Long subscriptionId;


    public UserPromoCode(Long uid, @Nonnull String provider, String code, PricingOptionType pricingOptionType,
                         @Nullable Integer subscriptionPeriod, @Nullable PricingOption subscriptionPricingOption,
                         @Nullable Long subscriptionId) {
        this.uid = uid;
        this.provider = provider;
        this.code = code;
        this.pricingOptionType = pricingOptionType;
        this.subscriptionPeriod = subscriptionPeriod;
        this.subscriptionPricingOption = subscriptionPricingOption;
        this.subscriptionId = subscriptionId;
        this.activatedAt = LocalDateTime.now();
    }


    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        UserPromoCode that = (UserPromoCode) o;
        return Objects.equals(uid, that.uid) &&
                Objects.equals(code, that.code) &&
                Objects.equals(provider, that.provider) &&
                pricingOptionType == that.pricingOptionType &&
                Objects.equals(subscriptionPeriod, that.subscriptionPeriod) &&
                Objects.equals(subscriptionPricingOption, that.subscriptionPricingOption) &&
                Objects.equals(subscriptionId, that.subscriptionId);
    }

    @Override
    public int hashCode() {
        int result = uid.hashCode();
        result = 31 * result + code.hashCode();
        result = 31 * result + (provider != null ? provider.hashCode() : 0);
        return result;
    }
}
