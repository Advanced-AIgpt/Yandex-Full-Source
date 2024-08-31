package ru.yandex.quasar.billing.beans;

import javax.annotation.Nullable;
import javax.validation.constraints.NotEmpty;

import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class DeliveryInfo {
    // productId of PricingOptionLine with delivery price and NDS
    @NotEmpty
    private final String productId;

    @NotEmpty
    private final String city;
    @Nullable
    private final String settlement;
    @Nullable
    private final String index;
    @NotEmpty
    private final String street;
    @NotEmpty
    private final String house;
    @Nullable
    private final String housing;
    @Nullable
    private final String building;
    @Nullable
    private final String porch;
    @Nullable
    private final String floor;
    @Nullable
    private final String flat;
}
