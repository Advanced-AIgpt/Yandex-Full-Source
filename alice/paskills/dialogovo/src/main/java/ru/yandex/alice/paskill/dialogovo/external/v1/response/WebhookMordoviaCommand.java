package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Collections;
import java.util.Map;

import javax.annotation.Nullable;
import javax.validation.constraints.NotNull;

import lombok.Data;

@Data
public class WebhookMordoviaCommand {
    @NotNull
    private final String command;

    @Nullable
    private final Map<String, ?> meta;

    public Map<String, ?> getMeta() {
        return meta != null ? meta : Collections.emptyMap();
    }
}
