package ru.yandex.quasar.billing.providers;

import org.hamcrest.Matcher;

import ru.yandex.quasar.billing.beans.PricingOption;

import static com.google.common.collect.Lists.newArrayList;
import static org.hamcrest.Matchers.allOf;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.everyItem;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.hasProperty;
import static org.hamcrest.Matchers.isEmptyString;
import static org.hamcrest.Matchers.not;
import static org.hamcrest.Matchers.notNullValue;
import static org.hamcrest.collection.IsCollectionWithSize.hasSize;

public final class ProviderIntegrationTestUtil {

    private ProviderIntegrationTestUtil() {
        throw new UnsupportedOperationException();
    }

    public static Matcher<PricingOption> reasonablePricingOption() {
        return allOf(
                newArrayList(
                        hasProperty("price", notNullValue()),
                        hasProperty("userPrice", notNullValue()),
                        hasProperty("type", notNullValue()),
                        hasProperty("purchasingItem", notNullValue()),
                        hasProperty("title", not(isEmptyString())),
                        hasProperty("providerPayload", not(isEmptyString())),
                        hasProperty("provider", not(isEmptyString())),
                        hasProperty("currency", equalTo("RUB"))  // TODO: probably un-hardcode that eventually
                )
        );
    }

    public static Matcher<ProviderPricingOptions> reasonablePricingOptions() {
        // TODO: match for more fields

        return allOf(
                notNullValue(),
                hasProperty("pricingOptions", everyItem(reasonablePricingOption())),
                hasProperty("pricingOptions", hasSize(greaterThan(0)))
        );
    }
}
