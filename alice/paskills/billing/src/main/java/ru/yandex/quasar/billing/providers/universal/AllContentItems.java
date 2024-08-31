package ru.yandex.quasar.billing.providers.universal;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class AllContentItems {
    @JsonProperty("content_items")
    private final List<String> contentItems;
}
