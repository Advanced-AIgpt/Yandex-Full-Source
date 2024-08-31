package ru.yandex.alice.paskill.dialogovo.test;

import java.util.Optional;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import org.springframework.http.HttpHeaders;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.UtteranceWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;

@RestController
public class TestSkill {
    public static final String COMMAND_NORMAL = "давай поиграем в города";
    public static final String COMMAND_TIMEOUT = "таймаут";

    public static final String RESPONSE_NORMAL = "Давай! Москва.";

    @PostMapping("/testSkill")
    public WebhookResponse process(
            @RequestBody UtteranceWebhookRequest request,
            @RequestHeader(HttpHeaders.AUTHORIZATION) @Nullable String authToken,
            @RequestHeader("X-RTLog-Token") @Nullable String rtLogToken
    ) {
        if (rtLogToken != null) {
            throw new RuntimeException("RtLogToken should always be null in webhook requests");
        }
        String utterance = request.getRequest().getOriginalUtterance();
        if (COMMAND_NORMAL.equals(utterance)) {
            return buildResponse(request, RESPONSE_NORMAL);
        } else if (COMMAND_TIMEOUT.equals(utterance)) {
            try {
                TimeUnit.SECONDS.sleep(10L);
            } catch (InterruptedException ignore) {
            }
            return buildResponse(request, RESPONSE_NORMAL);
        }

        throw new RuntimeException("unsupported request: " + request.toString());
    }

    private WebhookResponse buildResponse(UtteranceWebhookRequest request, String text) {
        var response = Response.builder()
                .endSession(false)
                .text(text)
                .tts(text)
                .endSession(false)
                .build();

        return WebhookResponse.builder()
                .response(Optional.of(response))
                .version(request.getVersion())
                .build();
    }
}
