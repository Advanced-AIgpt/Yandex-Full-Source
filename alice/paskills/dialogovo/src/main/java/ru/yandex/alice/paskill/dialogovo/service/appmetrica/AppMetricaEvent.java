package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Instant;
import java.util.List;
import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;

@Data
@Builder
public class AppMetricaEvent {
    private final ClientInfo clientInfo;
    @Builder.Default
    private final Optional<LocationInfo> locationInfo = Optional.empty();
    private final Session session;
    private final Instant eventTime;
    private final SkillInfo.Look skillLook;
    private final String skillId;
    @Builder.Default
    private final Optional<String> uid = Optional.empty();
    private final boolean endSession;
    @JsonProperty("metrica_events")
    private final List<MetricaEvent> events;
}
