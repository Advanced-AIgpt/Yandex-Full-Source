package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;

@Getter
public class SkillProductActivationFailedRequest extends RequestBase {
    private final SkillProductActivationErrorType error;

    public SkillProductActivationFailedRequest(SkillProductActivationErrorType error) {
        super(InputType.SKILL_PRODUCT_ACTIVATION_FAILED);
        this.error = error;
    }
}
