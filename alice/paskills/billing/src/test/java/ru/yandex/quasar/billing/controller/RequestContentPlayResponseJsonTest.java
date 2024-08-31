package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.util.Collections;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.quasar.billing.providers.RejectionReason;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.services.OfferCardData;

import static org.assertj.core.api.Assertions.assertThat;


@ExtendWith(SpringExtension.class)
@JsonTest
class RequestContentPlayResponseJsonTest {

    @Autowired
    private JacksonTester<RequestContentPlayResponse> json;

    @Test
    void testAvailableSerialization() throws IOException {
        var request = RequestContentPlayResponse.availableResponse(Map.of("test", StreamData.create("url", "{}")));

        assertThat(json.write(request))
                .isEqualToJson("{\"status\":\"available\",\"providers\":{\"test\": {\"url\": \"url\", \"payload\": " +
                        "{}}}}");
    }

    @Test
    void testAvailableSerializationNullStreamData() throws IOException {
        var request = RequestContentPlayResponse.availableResponse(Map.of("test", StreamData.EMPTY));

        assertThat(json.write(request))
                .isEqualToJson("{\"status\":\"available\",\"providers\":{\"test\": {}}}");
    }

    @Test
    void testPaymentRequiredSerialization() throws IOException {
        var request = RequestContentPlayResponse.paymentRequestedResponse(false, Map.of("provider_name",
                RejectionReason.PURCHASE_NOT_FOUND));

        assertThat(json.write(request))
                .isEqualToJson("{\"status\":\"payment_required\",\"any_active_subscription_exists\":false, " +
                        "\"provider_rejection_reasons\": {\"provider_name\": \"PURCHASE_NOT_FOUND\"}}");
    }

    @Test
    void testPaymentRequiredSerializationWithPersonalCard() throws IOException {
        var request = RequestContentPlayResponse.paymentRequestedResponse(false, Map.of("provider_name",
                RejectionReason.PURCHASE_NOT_FOUND), getOfferCardData(Collections.emptyMap()));

        assertThat(json.write(request))
                .isEqualToJson("{\"status\":\"payment_required\",\"any_active_subscription_exists\":false," +
                        "\"personal_card\":{\"card\":{\"card_id\":\"1\",\"button_url\":\"http\",\"text\":\"title\"," +
                        "\"date_from\":122,\"date_to\":455,\"yandex.station_film\":{}},\"remove_existing_cards\":" +
                        "true},\"provider_rejection_reasons\":{\"provider_name\":\"PURCHASE_NOT_FOUND\"}}");
    }

    @Test
    void testLoginRequiredSerialization() throws IOException {
        var request = RequestContentPlayResponse.loginRequiredResponse(Set.of("test"));

        assertThat(json.write(request))
                .isEqualToJson("{\"status\":\"provider_login_required\",\"providers_to_login\":[\"test\"]}");
    }

    @Test
    void testLoginRequiredSerializationWithPersonalCard() throws IOException {
        var request = RequestContentPlayResponse.loginRequiredResponse(Set.of("test"),
                getOfferCardData(Map.of("min_price", 3L)));

        assertThat(json.write(request))
                .isEqualToJson("{\"status\":\"provider_login_required\",\"providers_to_login\":[\"test\"]," +
                        "\"personal_card\":{\"card\":{\"card_id\":\"1\",\"button_url\":\"http\",\"text\":\"title\"," +
                        "\"date_from\":122,\"date_to\":455,\"yandex.station_film\":{\"min_price\":3}}," +
                        "\"remove_existing_cards\":true}}");
    }

    private OfferCardData getOfferCardData(Map<String, Object> params) {
        return new OfferCardData("1", "http", "title", 122, 455, params);
    }
}
