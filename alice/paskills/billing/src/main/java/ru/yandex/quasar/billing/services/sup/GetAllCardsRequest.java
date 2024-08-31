package ru.yandex.quasar.billing.services.sup;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class GetAllCardsRequest {
    private final Auth auth;
    @JsonProperty("client_id")
    private final String clientId = "search_app";

    public static GetAllCardsRequest create(String uid) {
        return new GetAllCardsRequest(new Auth(uid));
    }

}
