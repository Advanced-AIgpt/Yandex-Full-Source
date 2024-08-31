package ru.yandex.alice.paskill.dialogovo.processor;

import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.alice.paskill.dialogovo.external.WebhookError;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;

class DiagnosticInfoLogger {
    private static final Logger DIAGNOSTIC_INFO_LOG = LoggerFactory.getLogger("DIAGNOSTIC_INFO_LOG");

    private static final String DIAGNOSTIC_INFO_LOG_FORMAT = "tskv\ttskv_format={}\ttimestamp={}\ttimezone" +
            "={}\trequest_id={}\tskill_id={}\tsource={}\terrors={}\tsession_id={}\tuuid={}\tmsg_id={}";
    private static final DateTimeFormatter DIAGNISTIC_INFO_TIME_FORMATTER =
            DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss").withZone(ZoneId.of("UTC"));
    private static final DateTimeFormatter DIAGNISTIC_INFO_TIMEZONE_FORMATTER =
            DateTimeFormatter.ofPattern("Z").withZone(ZoneId.of("UTC"));

    public void log(Optional<String> requestIdO, String skillId, List<WebhookError> errors, SourceType source,
                    String sessionId, String uuid, long messageId) {
        Instant now = Instant.now();
        DIAGNOSTIC_INFO_LOG.trace(
                DIAGNOSTIC_INFO_LOG_FORMAT,
                "dialogovo/diagnostic-info-log",
                DIAGNISTIC_INFO_TIME_FORMATTER.format(now),
                DIAGNISTIC_INFO_TIMEZONE_FORMATTER.format(now),
                requestIdO.orElse(""),
                skillId,
                source.getCode(),
                escapeValue(errors.stream()
                        .map(error -> error.code().getCode())
                        .collect(Collectors.joining(","))),
                sessionId,
                uuid,
                messageId
        );
    }

    private String escapeValue(String value) {
        // See https://wiki.yandex-team.ru/statbox/LogRequirements/
        return defaultString(value)
                .replace("\\", "\\\\")
                .replace("\"", "\\\"")
                .replace("\t", "\\t")
                .replace("\n", "\\n")
                .replace("\r", "\\r")
                .replace("\0", "\\0");
    }

    private String defaultString(@Nullable String str) {
        return str == null ? "" : str;
    }
}
