package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState;

@Data
@JsonInclude(JsonInclude.Include.NON_ABSENT)
public class WebhookState {
    public static final WebhookState EMPTY = new WebhookState(
            Optional.empty(), Optional.empty(), Optional.empty(), Optional.empty(), Optional.empty()
    );

    @JsonProperty("canvas")
    private final Optional<Map<String, Object>> canvas;

    @JsonProperty("session")
    private final Optional<Map<String, Object>> session;

    @JsonProperty("user")
    private final Optional<Map<String, Object>> user;

    @JsonProperty("application")
    private final Optional<Map<String, Object>> application;

    @JsonProperty("audio_player")
    private final Optional<AudioPlayerState> audioPlayer;


    private WebhookState(
            Optional<Map<String, Object>> canvas,
            Optional<Map<String, Object>> session,
            Optional<Map<String, Object>> user,
            Optional<Map<String, Object>> application,
            Optional<AudioPlayerState> audioPlayer) {
        this.canvas = canvas;
        this.session = session;
        this.user = user;
        this.application = application;
        this.audioPlayer = audioPlayer;
    }

    @JsonCreator
    static WebhookState fromJson(
            @Nullable @JsonProperty("canvas") final Map<String, Object> canvas,
            @Nullable @JsonProperty("session") final Map<String, Object> session,
            @Nullable @JsonProperty("user") final Map<String, Object> user,
            @Nullable @JsonProperty("application") final Map<String, Object> application,
            @Nullable @JsonProperty("audio_player") final AudioPlayerState audioPlayerState
    ) {
        return new WebhookState(
                Optional.ofNullable(canvas),
                Optional.ofNullable(session),
                Optional.ofNullable(user),
                Optional.ofNullable(application),
                Optional.ofNullable(audioPlayerState)
        );
    }

    public static WebhookState create(
            Optional<Map<String, Object>> canvas,
            Optional<Map<String, Object>> session,
            Optional<Map<String, Object>> user,
            Optional<Map<String, Object>> application,
            Optional<AudioPlayerState> audioPlayer
    ) {
        if (canvas.isEmpty() && session.isEmpty() && user.isEmpty() && application.isEmpty() && audioPlayer.isEmpty()) {
            return EMPTY;
        } else {
            return new WebhookState(canvas, session, user, application, audioPlayer);
        }
    }

}
