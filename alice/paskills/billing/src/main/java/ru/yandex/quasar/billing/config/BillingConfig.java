package ru.yandex.quasar.billing.config;

import java.util.Collections;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.Setter;

@Setter
@Getter
public class BillingConfig {

    private AmediatekaConfig amediatekaConfig;

    private SocialAPIClientConfig socialAPIClientConfig;

    private String secretsConfigPath;

    private TrustBillingConfig trustBillingConfig;

    private SupPushServiceClientConfig supPushServiceClientConfig;

    private SecurityConfig securityConfig;

    private QuasarBackendConfig quasarBackendConfig;

    private DroidekaConfig droidekaConfig;

    private MusicApiConfig musicApiConfig;

    @JsonProperty("providersConfig")
    private Map<String, ProviderConfig> providersConfig;

    private TvmConfig tvmConfig;

    private Map<String, UniversalProviderConfig> universalProviders;

    private YaPayConfig yaPayConfig;
    private LaasConfig laasConfig;

    private String storePurchasePath;

    @JsonProperty("deviceTagToProvidedSkillProduct")
    private Map<String, List<SkillProductKeyConfig>> deviceTagToProvidedSkillProduct;

    // map to change user's IP
    @JsonProperty(required = false)
    private Map<String, String> userIpStub = Collections.emptyMap();
}
