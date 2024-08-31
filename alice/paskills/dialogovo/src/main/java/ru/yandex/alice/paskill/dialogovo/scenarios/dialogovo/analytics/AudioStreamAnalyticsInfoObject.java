package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import javax.annotation.Nonnull;

import lombok.Getter;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TAudioStream;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject;

@Getter
public class AudioStreamAnalyticsInfoObject extends AnalyticsInfoObject {

    private final String token;
    private final long offsetMs;

    public AudioStreamAnalyticsInfoObject(String token, long offsetMs) {
        super("external_skill.audio_player.stream", token, String.format("Аудио стрим " +
                "token=[%s] offset=[%d]", token, offsetMs));
        this.token = token;
        this.offsetMs = offsetMs;
    }

    @Nonnull
    @Override
    public TObject.Builder fillProtoField(@Nonnull TObject.Builder protoBuilder) {
        var payload = TAudioStream.newBuilder()
                .setToken(token)
                .setOffsetMs(offsetMs);
        return protoBuilder.setAudioStream(payload);
    }
}
