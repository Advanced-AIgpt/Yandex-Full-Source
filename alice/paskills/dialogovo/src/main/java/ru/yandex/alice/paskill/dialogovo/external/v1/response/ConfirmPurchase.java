package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Data;

@Data
@JsonSerialize
public class ConfirmPurchase {
    public static final ConfirmPurchase INSTANCE = new ConfirmPurchase();

    private ConfirmPurchase() { }
}
