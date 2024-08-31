package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import lombok.Data;

@Data
public class BindingInfo {
    private final Status status;
    private final String paymentMethodId;


    public enum Status {
        success,
        wait_for_notification,
        not_enough_funds,
        payment_not_found,
        @JsonEnumDefaultValue
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
