package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class PenguinaryRequest {
    @JsonProperty("node_id")
    private final String nodeId;
    private final String utterance;
}
