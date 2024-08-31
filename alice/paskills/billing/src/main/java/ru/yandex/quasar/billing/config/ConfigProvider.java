package ru.yandex.quasar.billing.config;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Collections;
import java.util.Map;
import java.util.Objects;

import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.util.Strings;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.quasar.billing.services.tvm.TvmToolConfig;

import static java.util.stream.Collectors.toMap;

@Configuration
public class ConfigProvider {

    private final BillingConfig billingConfig;

    @Value("${PG_DATABASE_URL:}")
    private String pgReadWriteDatabaseUrl;
    @Value("${PG_DATABASE_URL_RO:}")
    private String pgReadOnlyDatabaseUrl;

    @Value("${PG_USER:}")
    private String pgUser;
    @Value("${PG_PASSWORD:}")
    private String pgPassword;

    @Value("${XIVA_TOKEN:}")
    private String xivaToken;

    @Value("${tvm.config.path:}")
    private String tvmConfigJsonPath;

    // not used any more. leave null
    @Deprecated
    private String amediatekaClientSecret = "";
    @Deprecated
    private String purchaseServiceToken = "";

    public ConfigProvider(
            @Value("${quasar.config.path:quasar-billing.cfg}") String quasarConfigPath,
            @Value("${QUASAR_HOST_RU}") String quasarHostRu,
            @Value("${QUASAR_HOST_NET}") String quasarHostNet,
            @Value("${CALLBACK_HOST}") String callbackBaseUrl,
            @Value("${QLOUD_TVM_TOKEN:}") String qloudTvmToken,
            @Value("${TVM_TOKEN:}") String tvmToken
    ) throws IOException {
        String config = Files.readString(new File(quasarConfigPath).toPath());

        config = config.replace("${QUASAR_HOST_RU}", quasarHostRu)
                .replace("${QUASAR_HOST_NET}", quasarHostNet)
                .replace("${CALLBACK_HOST}", callbackBaseUrl);

        billingConfig = new ObjectMapper().readValue(config, BillingConfig.class);
        String token = Strings.isEmpty(qloudTvmToken) ? Objects.requireNonNull(Strings.trimToNull(tvmToken)) :
                qloudTvmToken;
        billingConfig.getTvmConfig().setTvmToken(token);

    }

    @Bean
    public BillingConfig billingConfig() {
        return billingConfig;
    }

    @Bean
    public SecretsConfig secretsConfig() throws IOException {
        var secretsFile = new File(billingConfig.getSecretsConfigPath());
        if (secretsFile.exists()) {
            ObjectMapper mapper = new ObjectMapper()
                    .configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);

            SecretsConfig secretsConfig = mapper
                    .readValue(new File(billingConfig.getSecretsConfigPath()), SecretsConfig.class);

            if (tvmConfigJsonPath != null) {
                TvmToolConfig tvmToolConfig = mapper.readValue(new File(tvmConfigJsonPath), TvmToolConfig.class);
                secretsConfig.setTvmAliases(tvmToolConfig.getClients().get("quasar-billing").getDsts()
                        .entrySet()
                        .stream()
                        .collect(toMap(Map.Entry::getKey, e -> e.getValue().getDstId()))
                );
            }
            return secretsConfig;
        } else {
            return new SecretsConfig(
                    pgReadWriteDatabaseUrl,
                    pgReadOnlyDatabaseUrl,
                    pgUser,
                    pgPassword,
                    xivaToken,
                    amediatekaClientSecret,
                    purchaseServiceToken,
                    Collections.emptyMap()
            );
        }
    }
}
