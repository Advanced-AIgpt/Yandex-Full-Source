package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import java.util.Optional;

import lombok.Data;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;
import ru.yandex.alice.paskills.common.billing.model.api.SkillProductItem;

import static ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SkillProductActivationActions.ACTIVATED_BY_MUSIC_ACTION;
import static ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SkillProductActivationActions.ACTIVATION_FAILED_MUSIC_NOT_PLAYING_ACTION;
import static ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SkillProductActivationActions.ACTIVATION_FAILED_MUSIC_NOT_RECOGNIZED_ACTION;

@Data
public class SkillProductActivationEvent {
    private final EventType type;
    private final Optional<SkillProductItem> skillProductItem;

    public SkillProductActivationEvent(EventType type, Optional<SkillProductItem> skillProductItem) {
        this.type = type;
        if (type == EventType.SUCCESS) {
            skillProductItem.orElseThrow(IllegalStateException::new);
        }
        this.skillProductItem = skillProductItem;
    }

    public enum EventType {
        SUCCESS,
        MUSIC_NOT_RECOGNIZED,
        MUSIC_NOT_PLAYING;

        public SkillProductActivationErrorType convertToError() {
            switch (this) {
                case MUSIC_NOT_PLAYING:
                    return SkillProductActivationErrorType.MUSIC_NOT_PLAYING;
                case MUSIC_NOT_RECOGNIZED:
                    return SkillProductActivationErrorType.MUSIC_NOT_RECOGNIZED;
                default:
                    return SkillProductActivationErrorType.MEDIA_ERROR_UNKNOWN;
            }
        }

        public AnalyticsInfoAction convertToActionEvent() {
            switch (this) {
                case SUCCESS:
                    return ACTIVATED_BY_MUSIC_ACTION;
                case MUSIC_NOT_RECOGNIZED:
                    return ACTIVATION_FAILED_MUSIC_NOT_RECOGNIZED_ACTION;
                case MUSIC_NOT_PLAYING:
                    return ACTIVATION_FAILED_MUSIC_NOT_PLAYING_ACTION;
                default:
                    throw new IllegalStateException("Unknown skill product activation event type: " + this);
            }
        }
    }
}
