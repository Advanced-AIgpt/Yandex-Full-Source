package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonValue;

public enum TemplateTag {
    DESKTOP("desktop/form"),
    MOBILE("mobile/form"),
    STARTTV("smarttv/form");

    private final String tag;

    TemplateTag(String tag) {
        this.tag = tag;
    }

    @JsonValue
    public String getTag() {
        return tag;
    }
}
