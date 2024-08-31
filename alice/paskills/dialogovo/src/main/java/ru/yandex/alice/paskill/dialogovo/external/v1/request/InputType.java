package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonValue;

public enum InputType {
    SIMPLE_UTTERANCE("SimpleUtterance"),
    BUTTON_PRESSED("ButtonPressed"),
    AUDIO_PLAYER_PLAYBACK_STARTED("AudioPlayer.PlaybackStarted", "audio_player_started"),
    AUDIO_PLAYER_PLAYBACK_FINISHED("AudioPlayer.PlaybackFinished", "audio_player_finished"),
    AUDIO_PLAYER_PLAYBACK_NEARLY_FINISHED("AudioPlayer.PlaybackNearlyFinished", "audio_nearly_finished"),
    AUDIO_PLAYER_PLAYBACK_STOPPED("AudioPlayer.PlaybackStopped", "audio_player_stopped"),
    AUDIO_PLAYER_PLAYBACK_FAILED("AudioPlayer.PlaybackFailed", "audio_player_failed"),
    PURCHASE_CONFIRMATION("Purchase.Confirmation"),
    PURCHASE_COMPLETE("Purchase.Complete"),
    SKILL_PRODUCT_ACTIVATED("SkillProduct.Activated"),
    SKILL_PRODUCT_ACTIVATION_FAILED("SkillProduct.ActivationFailed"),
    SHOW_PULL_REQUEST("Show.Pull"),
    GEOLOCATION_ALLOWED("Geolocation.Allowed"),
    GEOLOCATION_REJECTED("Geolocation.Rejected"),
    USER_AGREEMENTS_ACCEPTED("UserAgreements.Accepted"),
    USER_AGREEMENTS_REJECTED("UserAgreements.Rejected"),
    WIDGET_GALLERY("WidgetGallery"),
    TEASERS("Teasers");

    private final String code;
    private final String metricaValue;

    InputType(String code) {
        this(code, null);
    }

    InputType(String code, String metricaValue) {
        this.code = code;
        this.metricaValue = metricaValue;
    }

    @JsonValue
    public String getCode() {
        return code;
    }

    @JsonIgnore
    public String getMetricaValue() {
        return metricaValue;
    }
}
