package ru.yandex.quasar.billing.config;

import java.util.Map;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;

@AllArgsConstructor
@Getter
public class SecretsConfig {

    private final String pgReadWriteDatabaseUrl;
    private final String pgReadOnlyDatabaseUrl;
    private final String pgUser;
    private final String pgPassword;
    private final String xivaToken;
    private final String amediatekaClientSecret;
    private final String purchaseServiceToken;
    @Setter(AccessLevel.PACKAGE)
    private Map<String, Integer> tvmAliases;
}
