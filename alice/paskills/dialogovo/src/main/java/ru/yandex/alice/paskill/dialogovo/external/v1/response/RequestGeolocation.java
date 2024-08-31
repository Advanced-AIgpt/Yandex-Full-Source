package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Data;

@Data
@JsonSerialize
public class RequestGeolocation {
    public static final RequestGeolocation INSTANCE = new RequestGeolocation();

    private RequestGeolocation() { }
}
