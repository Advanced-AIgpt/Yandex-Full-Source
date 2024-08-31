package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import java.util.Optional;

import javax.annotation.Nonnull;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillUser;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject;

public class UserAnalyticsInfoObject extends AnalyticsInfoObject {
    private final String userId;
    private final Optional<String> persistentId;

    public UserAnalyticsInfoObject(String userId, Optional<String> persistentId) {
        super("external_skill.user", "external_skill.user", "Пользователь навыка");
        this.userId = userId;
        this.persistentId = persistentId;
    }

    public String getUserId() {
        return userId;
    }

    public Optional<String> getPersistentId() {
        return persistentId;
    }

    @Nonnull
    @Override
    public TObject.Builder fillProtoField(@Nonnull TObject.Builder protoBuilder) {
        var payload = TSkillUser.newBuilder()
                .setSkillUserId(userId)
                .setPersistentSkillUserId(persistentId.orElse(""));
        return protoBuilder.setSkillUser(payload);
    }
}
