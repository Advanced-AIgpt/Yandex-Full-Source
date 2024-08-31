package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import javax.validation.constraints.NotNull;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import lombok.Data;
import lombok.NoArgsConstructor;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@NoArgsConstructor
@JsonTypeInfo(
        use = JsonTypeInfo.Id.NAME,
        property = "action",
        visible = true)
@JsonSubTypes({
        @JsonSubTypes.Type(value = Play.class, name = "Play"),
        @JsonSubTypes.Type(value = Stop.class, name = "Stop"),
        @JsonSubTypes.Type(value = Rewind.class, name = "Rewind"),
})
@JsonInclude(NON_ABSENT)
public abstract class AudioPlayerAction {
    @NotNull
    private AudioPlayerActionType action;
}
