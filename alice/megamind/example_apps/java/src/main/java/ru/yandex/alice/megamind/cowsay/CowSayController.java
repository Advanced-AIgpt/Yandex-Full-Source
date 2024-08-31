package ru.yandex.alice.megamind.cowsay;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.MessageOrBuilder;
import com.google.protobuf.util.JsonFormat;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpHeaders;
import org.springframework.lang.Nullable;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame;
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame.TSlot;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioRunRequest;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TLayout;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TLayout.TCard;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioResponseBody;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse.TFeatures;

import static ru.yandex.alice.megamind.cowsay.CowSayController.APPLICATION_PROTOBUF;
import static ru.yandex.alice.megamind.cowsay.CowSayController.APPLICATION_X_PROTOBUF;

@RestController
@RequestMapping(consumes = {MimeTypeUtils.APPLICATION_JSON_VALUE, APPLICATION_PROTOBUF, APPLICATION_X_PROTOBUF})
public class CowSayController {
    static final String APPLICATION_PROTOBUF = "application/protobuf";
    static final String APPLICATION_X_PROTOBUF = "application/x-protobuf";

    private static final Logger logger = LogManager.getLogger();

    private final JsonFormat.Printer printer;
    private static final String INTENT_NAME = "alice.cowsay";

    public CowSayController() {
        this.printer = JsonFormat.printer();
    }

    @PostMapping(value = "/cowsay/run")
    public TScenarioRunResponse run(
            @RequestBody TScenarioRunRequest request,
            @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) @Nullable String contentType,
            @RequestHeader(value = HttpHeaders.ACCEPT, required = false) @Nullable String accept
    ) {

        logger.info("Run request {}, {}: {}", contentType, accept, toString(request));

        TScenarioRunResponse response;
        try {
            response = runImpl(request);
        } catch (Exception e) {
            response = createTextVoiceResponse("Не могу сказать, что то пошло не так!");
            logger.error("Failed to handle run request", e);
        }

        logger.info("Run response: {}", toString(response));
        return response;
    }

    private TScenarioRunResponse runImpl(TScenarioRunRequest request) {
        boolean isFound = false;
        TSemanticFrame cowSayFrame = null;
        for (TSemanticFrame frame : request.getInput().getSemanticFramesList()) {
            if (INTENT_NAME.equals(frame.getName())) {
                isFound = true;
                cowSayFrame = frame;
                break;
            }
        }
        if (!isFound) {
            return TScenarioRunResponse.newBuilder()
                    .setFeatures(TFeatures.newBuilder().setIsIrrelevant(true))
                    .build();
        }

        TSlot animalSlot = null;
        for (TSlot slot : cowSayFrame.getSlotsList()) {
            if ("animal".equals(slot.getName())) {
                animalSlot = slot;
                break;
            }
        }

        String text = null;
        switch (animalSlot.getValue()) {
            case "cow": {
                text = "Муу!";
                break;
            }
            case "dog": {
                text = "Гав!";
                break;
            }
            case "cat": {
                text = "Мяу!";
                break;
            }
        }
        return createTextVoiceResponse(text);
    }

    private TScenarioRunResponse createTextVoiceResponse(String textAndVoice) {
        TCard card = TCard.newBuilder().setText(textAndVoice).build();
        TLayout layout = TLayout.newBuilder()
                .setOutputSpeech(textAndVoice)
                .addCards(card)
                .build();
        TScenarioResponseBody responseBody = TScenarioResponseBody.newBuilder()
                .setLayout(layout)
                .build();
        return TScenarioRunResponse.newBuilder()
                .setResponseBody(responseBody)
                .build();
        // NOTE: response doesn't have ApplyArguments, so it's a pure scenario
    }

    private String toString(MessageOrBuilder msg) {
        try {
            return printer.print(msg);
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException(e);
        }
    }
}
