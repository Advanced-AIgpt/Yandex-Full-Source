package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;

@Getter
public class SkillProductActivatedRequest extends RequestBase {
    @JsonProperty("product_uuid")
    private final String productUuid;

    @JsonProperty("product_name")
    private final String productName;

    public SkillProductActivatedRequest(String productUuid, String productName) {
        super(InputType.SKILL_PRODUCT_ACTIVATED);
        this.productUuid = productUuid;
        this.productName = productName;
    }
}
