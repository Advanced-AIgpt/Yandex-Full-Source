package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import javax.validation.constraints.NotNull;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class WebhookMordoviaShow {
    //TODO: think about vulnerabilities if cross-domain URL provided
    // see: https://a.yandex-team.ru/review/1079820/details#comment-1596803
    @NotNull
    private final String url;
    @JsonProperty("is_full_screen")
    private final boolean isFullScreen;
}
