package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import java.util.Map;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Data;

@Data
public class CanvasCallback {
    private final String command;
    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    private final Map<String, ?> meta;
}
