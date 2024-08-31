package ru.yandex.quasar.billing.providers;

import org.springframework.stereotype.Component;

@Component
public class TestContentProvider extends DelegatingContentProvider {
    public static final String PROVIDER_NAME = "testProvider";
    public static final Long PARTNER_ID = 100L; // defined in /configs/dev/quasar-billing.cfg
    public static final String PROVIDER_SOCIAL_NAME = "testProviderSocial";
    public static final String PROVIDER_TOKEN = "testProviderToken";

    public TestContentProvider() {
        super(PROVIDER_NAME, PROVIDER_SOCIAL_NAME, null);
    }
}
