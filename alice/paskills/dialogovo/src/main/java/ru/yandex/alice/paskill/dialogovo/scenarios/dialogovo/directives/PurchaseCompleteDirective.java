package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.Directive;

@Data
@Directive("external_skill__purchase_complete")
public class PurchaseCompleteDirective implements CallbackDirective {
    @JsonProperty("skill_id")
    private final String skillId;

    @JsonProperty("purchase_offer_uuid")
    private final String purchaseOfferUuid;

    @JsonProperty("initial_device_id")
    private final String initialDeviceId;

    @JsonProperty("source")
    private final Optional<String> source;

}
