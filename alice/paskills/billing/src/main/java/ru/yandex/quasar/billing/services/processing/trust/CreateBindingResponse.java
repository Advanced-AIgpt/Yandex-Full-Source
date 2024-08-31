package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class CreateBindingResponse {

    private final Status status;
    @JsonProperty("purchase_token")
    private final String purchaseToken;

    public enum Status {
        success,
        unknown;

        @JsonCreator
        @Nullable
        public static Status forName(String name) {
            for (Status item : Status.values()) {
                if (item.name().equals(name)) {
                    return item;
                }
            }
            return name != null ? unknown : null;
        }

    }

}
