package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class Markup {
    private static final Markup DANGEROUS_MARKUP = new Markup(true);
    private static final Markup SAFE_MARKUP = new Markup(false);

    @JsonProperty("dangerous_context")
    private final boolean dangerousContext;

    public static Markup of(boolean dangerousContext) {
        return dangerousContext ? DANGEROUS_MARKUP : SAFE_MARKUP;
    }
}
