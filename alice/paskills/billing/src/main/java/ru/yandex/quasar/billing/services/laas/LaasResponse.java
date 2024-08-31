package ru.yandex.quasar.billing.services.laas;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
class LaasResponse {
    @JsonProperty("region_id")
    private final Integer regionId;
    @JsonProperty("is_anonymous_vpn")
    private final boolean isAnonymousVpn;
    @JsonProperty("is_public_proxy")
    private final boolean isPublicProxy;
    @JsonProperty("region_by_ip")
    private final Integer regionByIp;
    @JsonProperty("is_user_choice")
    private final boolean isUserChoice;
    @JsonProperty("country_id_by_ip")
    private final Integer countryId;
}
