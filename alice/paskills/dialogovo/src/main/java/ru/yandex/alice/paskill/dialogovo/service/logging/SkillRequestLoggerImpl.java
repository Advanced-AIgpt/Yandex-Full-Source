package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.time.Instant;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.UUID;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.AnnotationIntrospector;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.introspect.AnnotationIntrospectorPair;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.util.Strings;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.external.WebhookError;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestParams;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult;

import static java.util.Collections.emptyList;

@Component
class SkillRequestLoggerImpl implements SkillRequestLogger {

    private static final Logger logger = LogManager.getLogger();

    private final SkillRequestLogPersistent skillRequestLogPersistent;
    private final ObjectMapper objectMapper;

    /**
     * Special ObjectMapper is constructed from default one to mask sensitive data
     *
     * @see MaskSensitiveDataAnnotationIntrospector
     */
    SkillRequestLoggerImpl(
            SkillRequestLogPersistent skillRequestLogPersistent,
            ObjectMapper objectMapper
    ) {
        this.skillRequestLogPersistent = skillRequestLogPersistent;

        ObjectMapper mapper = objectMapper.copy();
        AnnotationIntrospector origIntrospector = mapper.getSerializationConfig().getAnnotationIntrospector();
        AnnotationIntrospector newIntrospector = AnnotationIntrospectorPair.pair(origIntrospector,
                new MaskSensitiveDataAnnotationIntrospector());
        mapper.setAnnotationIntrospectors(newIntrospector,
                mapper.getDeserializationConfig().getAnnotationIntrospector());
        this.objectMapper = mapper;
    }

    @Override
    public void log(WebhookRequestParams request, WebhookRequestResult response) {


        LogRecord.Status status = getStatus(response);
        List<String> validationErrors = status == LogRecord.Status.VALIDATION_ERROR ?
                response.getErrors().stream()
                        .map(err -> Objects.requireNonNullElse(err.message(), "ERROR") +
                                Optional.ofNullable(err.path()).map(path -> ": " + path).orElse(""))
                        .collect(Collectors.toList()) : emptyList();

        LogRecord record = new LogRecord(
                request.getSkill().getId(),
                request.getBody().getSession().getSessionId(),
                request.getBody().getSession().getMessageId(),
                request.getBody().getSession().getEventId().map(UUID::toString).orElse(null),
                Instant.now(),
                requestToString(request),
                response.getRawResponse().orElse(null),
                status,
                validationErrors,
                response.getCallDuration().toMillis(),
                request.getInternalRequestId(),
                request.getInternalDeviceId(),
                request.getInternalUuid()
        );

        logger.debug("starting webhook request save task. skill_id: {}, session_id: {}, msg_id: {}",
                request.getSkill().getId(),
                request.getBody().getSession().getSessionId(),
                request.getBody().getSession().getMessageId()
        );

        skillRequestLogPersistent.save(record);
    }

    private LogRecord.Status getStatus(WebhookRequestResult response) {
        if (response.hasErrors()) {
            if (response.getErrors().size() == 1) {
                WebhookError error = response.getErrors().get(0);
                if (error.code().isFatal()) {
                    return LogRecord.Status.fromWebhookErrorCode(error.code()).orElse(LogRecord.Status.UNKNOWN);
                }
            }
            // size > 1 as we checked it `hasErrors()`
            return LogRecord.Status.VALIDATION_ERROR;
        } else {
            return LogRecord.Status.OK;
        }

    }

    @Nullable
    private String requestToString(WebhookRequestParams request) {
        try {
            return Strings.trimToNull(objectMapper.writeValueAsString(request.getBody()));
        } catch (JsonProcessingException e) {
            logger.warn("Unable to convert webhook request body to String", e);
            return null;
        }
    }
}
