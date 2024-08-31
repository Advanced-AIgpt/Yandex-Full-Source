package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@Data
class CreateBindingRequest {
    private final TrustCurrency currency;

    @JsonInclude(value = JsonInclude.Include.NON_NULL)
    @JsonProperty("back_url")
    @Nullable
    private final String backUrl;

    @JsonInclude(value = JsonInclude.Include.NON_NULL)
    @JsonProperty("return_path")
    @Nullable
    private final String returnPath;

    @JsonProperty("template_tag")
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private final TemplateTag templateTag;

}
