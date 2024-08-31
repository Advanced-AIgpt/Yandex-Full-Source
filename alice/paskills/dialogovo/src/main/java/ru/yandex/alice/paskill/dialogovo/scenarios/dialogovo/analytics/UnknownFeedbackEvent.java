package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import javax.annotation.Nonnull;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoEvent;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TEvent;

public class UnknownFeedbackEvent extends AnalyticsInfoEvent {

    public static final UnknownFeedbackEvent INSTANCE = new UnknownFeedbackEvent();

    private UnknownFeedbackEvent() {

    }

    @Nonnull
    @Override
    protected TEvent.Builder fillProtoField(@Nonnull TEvent.Builder protoBuilder) {
        return protoBuilder.setSkillFeedbackUnknownRating(Dialogovo.TFeedbackUnknownRatingEvent.getDefaultInstance());
    }
}
