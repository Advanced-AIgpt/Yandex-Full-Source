package ru.yandex.quasar.billing.services.sup;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class DeleteCardRequest {
    private final Auth auth;
    @JsonProperty("card_id")
    private final String cardId;
    // we don't use periodic cards so no need for the flag
    // @JsonProperty("delete_card")
    // private final boolean deleteCard = true;

    static DeleteCardRequest createDeleteCardRequest(String uid, String cardId) {
        return new DeleteCardRequest(new Auth(uid), cardId);
    }
}
