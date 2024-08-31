package ru.yandex.alice.paskill.dialogovo.providers;

import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;

public interface UserProvider {
    String getApplicationId(SkillInfo skill, String userId);

    /**
     * Returns empty if userId is null or persistent user id is not supported
     */
    Optional<String> getPersistentUserId(SkillInfo skill, @Nullable String userId);
}
