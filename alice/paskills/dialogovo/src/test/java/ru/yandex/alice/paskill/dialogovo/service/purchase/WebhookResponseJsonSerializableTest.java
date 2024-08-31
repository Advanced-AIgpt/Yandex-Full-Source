package ru.yandex.alice.paskill.dialogovo.service.purchase;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Optional;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.commons.io.IOUtils;
import org.junit.jupiter.api.Test;
import org.skyscreamer.jsonassert.JSONAssert;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import ru.yandex.alice.kronstadt.test.DynamicValueTokenComparator;
import ru.yandex.alice.paskill.dialogovo.external.ApiVersion;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ConfirmPurchase;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.StartAccountLinking;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Stop;

@JsonTest
public class WebhookResponseJsonSerializableTest {

    @Autowired
    private ObjectMapper objectMapper;

    @Test
    public void testSerialization() throws Exception {
        var response = WebhookResponse.builder()
                .startAccountLinking(Optional.of(new StartAccountLinking()))
                .response(Optional.of(Response.builder()
                        .text("TEXT")
                        .tts("TTS")
                        .directives(Optional.of(new Directives(
                                Optional.of(new Stop()),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.of(new StartAccountLinking()),
                                Optional.empty(),
                                Optional.of(ConfirmPurchase.INSTANCE),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.empty()
                        )))
                        .build()))
                .version(ApiVersion.V1_0)
                .build();

        String serializedResponse = objectMapper.writeValueAsString(response);

        String responseFile = "json/serialized_webhook_response.json";
        Resource resource = new PathMatchingResourcePatternResolver().getResource(responseFile);
        try (InputStream inputStream = resource.getInputStream()) {
            String actualResponse = IOUtils.toString(inputStream, StandardCharsets.UTF_8);
            JSONAssert.assertEquals(serializedResponse, actualResponse,
                    new DynamicValueTokenComparator(JSONCompareMode.STRICT));
        }
    }
}
