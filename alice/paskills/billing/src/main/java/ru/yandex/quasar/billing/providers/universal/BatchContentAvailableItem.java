package ru.yandex.quasar.billing.providers.universal;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
@JsonInclude(JsonInclude.Include.NON_NULL)
class BatchContentAvailableItem {
    @JsonProperty("content_item_id")
    private final String contentItemId;
    private final Boolean available;
    private final List<ProductItem> purchasables;
    @JsonProperty("error_code")
    private final Integer errorCode;
    @JsonProperty("error_text")
    private final String errorTest;

    public static BatchContentAvailableItemBuilder builder(String contentItemId) {
        return new BatchContentAvailableItemBuilder().contentItemId(contentItemId);
    }
}
