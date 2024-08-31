package ru.yandex.alice.paskill.dialogovo.domain;

public final class UserFeatureFlag {
    public static final String AUDIO_PLAYER = "audioPlayer";
    public static final String MORDOVIA = "mordovia";
    public static final String SKILL_PURCHASE = "skill_purchase";
    public static final String USER_SKILL_PRODUCT = "user_skill_product";
    public static final String GEOLOCATION_SHARING = "geolocation_sharing";
    public static final String USER_AGREEMENTS = "enableUserAgreements";

    private UserFeatureFlag() {
        throw new UnsupportedOperationException();
    }
}
