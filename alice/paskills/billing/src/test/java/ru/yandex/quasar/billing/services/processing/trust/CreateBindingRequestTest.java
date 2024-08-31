package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.quasar.billing.services.processing.trust.TemplateTag.MOBILE;

@JsonTest
@SpringJUnitConfig
class CreateBindingRequestTest {

    @Autowired
    private JacksonTester<CreateBindingRequest> tester;

    @Test
    void testSerialize() throws IOException {

        assertThat(tester.write(new CreateBindingRequest(TrustCurrency.RUB, "https://service.api.yandex" +
                ".ru/payment/notification", null, null)))
                .isEqualToJson("{\n" +
                        "  \"currency\": \"RUB\",\n" +
                        "  \"back_url\": \"https://service.api.yandex.ru/payment/notification\"" +
                        "}");
    }

    @Test
    void testSerializationNull() throws IOException {
        assertThat(tester.write(new CreateBindingRequest(TrustCurrency.RUB, null, null, null)))
                .isEqualToJson("{\n" +
                        "  \"currency\": \"RUB\"" +
                        "}");
    }

    @Test
    void testDeserialize() throws IOException {
        tester.parse("{\n" +
                "  \"currency\": \"RUB\",\n" +
                "  \"return_path\": \"http://ya.ru\",\n" +
                "  \"template_tag\": \"mobile/form\"\n" +
                "}").assertThat().isEqualTo(
                new CreateBindingRequest(TrustCurrency.RUB, null, "http://ya.ru", MOBILE)
        );
    }
}
