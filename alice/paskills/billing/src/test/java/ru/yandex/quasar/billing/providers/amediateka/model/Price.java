package ru.yandex.quasar.billing.providers.amediateka.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

@Getter
public class Price {
    private Integer id;
    private String uid;
    private Integer period;  // in days
    private String currency; // like "rub"
    private String value;  // value is given in strings like "1599.00"
    private Integer discount;  // in percent
    @JsonProperty("original_value")
    private String originalValue;  // pre-discount price
}
