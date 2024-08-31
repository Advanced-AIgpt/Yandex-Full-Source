package ru.yandex.alice.paskill.dialogovo.controller;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Strings;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Interfaces;
import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ResponseAnalytics;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.AliceHandledWithDevConsoleMessageException;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext.FloydRequestContext;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationService;
import ru.yandex.alice.paskill.dialogovo.vins.ExternalDevError;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookException;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult;
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmRequired;

@RestController
@TvmRequired("floyd")
class FloydController {

    private static final Logger logger = LogManager.getLogger();

    private final SkillRequestProcessor requestProcessor;
    private final NormalizationService normalizer;
    private final SkillProvider skillProvider;
    private final RequestContext requestContext;
    private final DialogovoRequestContext dialogovoRequestContext;

    static final String RANDOM_SEED_HEADER = "X-Test-Random-Seed";

    FloydController(SkillRequestProcessor requestProcessor,
                    NormalizationService normalizer,
                    SkillProvider skillProvider,
                    RequestContext requestContext,
                    DialogovoRequestContext dialogovoRequestContext) {
        this.requestProcessor = requestProcessor;
        this.normalizer = normalizer;
        this.skillProvider = skillProvider;
        this.requestContext = requestContext;
        this.dialogovoRequestContext = dialogovoRequestContext;
    }


    @PostMapping("/floyd/skill/{skillId}")
    public ResponseEntity<FloydResponse> floyd(
            @PathVariable String skillId,
            @RequestHeader(name = RANDOM_SEED_HEADER, required = false) @Nullable Long randomSeed,
            @RequestBody FloydRequest request
    ) {
        logger.debug("Request: {}", request);

        Optional<SkillInfo> skillO = skillProvider.getSkill(skillId);
        if (skillId.isEmpty()) {
            logger.warn("Can't find Floyd skill by {}", skillId);
            return ResponseEntity.badRequest().build();
        }
        SkillInfo skillInfo = skillO.get();

        Random r = randomSeed != null ? new Random(randomSeed) : new Random();

        SkillProcessRequest processReq = convert(request, skillInfo, r);
        var ctx = new Context(SourceType.USER);
        SkillProcessResult result = null;
        List<ExternalDevError> errors = List.of();

        if (request.userId.containsKey("puid") ||
                request.userId.containsKey("login") ||
                request.userId.containsKey("operator_chat_id")) {

            String puid = (String) request.userId.get("puid");
            String login = (String) request.userId.get("login");
            String operatorChatId = (String) request.userId.get("operator_chat_id");

            dialogovoRequestContext.setFloydRequestContext(new FloydRequestContext(puid, login, operatorChatId));

            if (puid != null) {
                requestContext.setCurrentUserId(puid);
            }
        }

        try {
            result = requestProcessor.process(ctx, processReq);
        } catch (WebhookException ex) {
            result = SkillProcessResult.builder(ex.getResponse(), processReq.getSkill(), ex.getRequest().getBody())
                    .build();

            errors = ex.getResponse().getErrors().stream()
                    .map(x -> new ExternalDevError(x.code().getCode(), x.code().getDescription(),
                            x.path(),
                            x.message()))
                    .collect(Collectors.toList());

        } catch (AliceHandledException ex) {
            // тут мы не можем понять ходили мы в вебхук или нет
            // возможно стоит добавить ответ в AliceHandledException
            // или отказаться от исключений в этом месте
            result = SkillProcessResult.builder(
                            Optional.ofNullable(result).flatMap(SkillProcessResult::getResponseO).orElse(null),
                            processReq.getSkill(),
                            Optional.ofNullable(result).map(SkillProcessResult::getWebhookRequest).orElse(null))
                    .build();

            if (ex instanceof AliceHandledWithDevConsoleMessageException debugEx) {
                errors = List.of(new ExternalDevError("error", debugEx.getExternalDebugMessage()));
            }
        } catch (Exception ex) {
            logger.error("Floyd controller detected not AliceHandledException. ", ex);
            throw ex;
        }

        FloydResponse response = convert(request, result, errors);
        logger.debug("Response: {}", response);

        return ResponseEntity.ok(response);
    }

    private FloydResponse convert(FloydRequest request,
                                  SkillProcessResult processResult,
                                  List<ExternalDevError> errors) {
        if (!errors.isEmpty()) {
            logger.warn("Errors processing request: {}", errors);
            return new FloydResponse(FloydStatus.error, "current", "Произошла какая-то ошибка", List.of(),
                    request.inputValues, null);
        } else {
            TextCard textCard = processResult.getLayout().joinTextCards();
            String gotoNode;
            if (processResult.getEndSession()) {
                gotoNode = Strings.isNullOrEmpty(processResult.getFloydExitNode()) ?
                        request.getGraphCurrentNode() + "_exit" :
                        processResult.getFloydExitNode();
            } else {
                gotoNode = "current";
            }
            return new FloydResponse(FloydStatus.ok,
                    gotoNode,
                    textCard != null ? textCard.getText() : null,
                    processResult.getLayout().getSuggests().stream()
                            .map(Button::getText)
                            .collect(Collectors.toList()),
                    processResult.getSession()
                            .filter(__ -> !processResult.getEndSession())
                            .map(InputValues::fromSession)
                            .orElse(null),
                    processResult.getResponseO()
                            .flatMap(WebhookRequestResult::getResponse)
                            .flatMap(WebhookResponse::getAnalytics)
                            .orElse(null)
            );
        }
    }

    private SkillProcessRequest convert(FloydRequest floydRequest, SkillInfo skillInfo, Random r) {
        return SkillProcessRequest.builder()
                .originalUtterance(floydRequest.inputText.orElse(""))
                .clientInfo(new ClientInfo(
                        "floyd",
                        "unknown-app-version",
                        "floyd",
                        "1.0",
                        "floyd-uuid",
                        null,
                        "ru-RU",
                        "Europe/Moscow",
                        "unknown-device-model",
                        "unknown-device-manufacturer",
                        Interfaces.getEMPTY()
                ))
                .normalizedUtterance(normalizer.normalize(floydRequest.inputText.orElse("")))
                .session(Optional.ofNullable(floydRequest.getInputValues())
                        .flatMap(InputValues::toSession))
                .skill(skillInfo)
                .isButtonPress(false)
                .voiceSession(false)
                .random(r)
                .activationSourceType(ActivationSourceType.FLOYD)
                .requestTime(Instant.now())
                .viewState(Optional.empty())
                .mementoData(RequestProto.TMementoData.getDefaultInstance())
                .build();
    }


    /*{
        "graph_id": <str>,
        "graph_current_node": <str>,
        "input_text": <str>,
        "input_values": {},  # все значения накопленные при обходе графа
        "user_id": {}
    }*/
    @Data
    static class FloydRequest {
        @JsonProperty("graph_id")
        private final String graphId;
        @JsonProperty("graph_current_node")
        private final String graphCurrentNode;
        @JsonProperty("input_text")
        private final Optional<String> inputText;

        // все значения накопленные при обходе графа
        @JsonProperty("input_values")
        private final InputValues inputValues;
        @JsonProperty("user_id")
        private final Map<String, Object> userId;
    }

    /*{
        "status": "ok",
        "goto_node": <str>,  # optional, default - current
        "show_message": <str>,  # optional
        "show_suggests": [<str>, ], # optional, имеет смысл в текущей ноде
        "set_values": {<str>, <str>}  # optional
    }*/
    @JsonInclude(JsonInclude.Include.NON_NULL)
    @Data
    static class FloydResponse {
        private final FloydStatus status;
        @JsonProperty("goto_node")
        private final String gotoNode;
        @JsonProperty("show_message")
        private final String message;
        @JsonProperty("show_suggests")
        private final List<String> suggests;
        @JsonProperty("set_values")
        private final InputValues values;
        @Nullable
        private final ResponseAnalytics analytics;
    }

    @Data
    static class InputValues {
        @JsonProperty("session_id")
        private final String sessionId;
        @JsonProperty("message_id")
        private final long messageId;
        @JsonProperty("start_timestamp")
        private final long startTimestamp;

        Optional<Session> toSession() {
            if (sessionId != null && !sessionId.isEmpty()) {
                return Optional.of(Session.create(sessionId,
                        messageId,
                        Instant.ofEpochMilli(startTimestamp),
                        false,
                        null,
                        ActivationSourceType.UNDETECTED
                ));
            } else {
                return Optional.empty();
            }
        }

        static InputValues fromSession(Session session) {
            return new InputValues(session.getSessionId(),
                    session.getMessageId(),
                    session.getStartTimestamp().toEpochMilli()
            );
        }
    }

    enum FloydStatus {
        ok,
        error
    }

}
