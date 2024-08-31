package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import java.util.Arrays;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.ToString;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoEvent;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TFeedbackSavedEvent;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TEvent;
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark;

@ToString
public class FeedbackSavedEvent extends AnalyticsInfoEvent {

    private static final Map<FeedbackMark, FeedbackSavedEvent> INSTANCES = Arrays.stream(FeedbackMark.values())
            .collect(Collectors.toMap(mark -> mark, FeedbackSavedEvent::new));

    private final FeedbackMark mark;

    private FeedbackSavedEvent(FeedbackMark mark) {
        super();
        this.mark = mark;
    }

    public FeedbackMark getMark() {
        return mark;
    }

    @Nonnull
    @Override
    protected TEvent.Builder fillProtoField(@Nonnull TEvent.Builder protoBuilder) {
        long rating = mark.getMarkValue();
        return protoBuilder.setSkillFeedbackSaved(TFeedbackSavedEvent.newBuilder().setRating(rating));
    }

    public static FeedbackSavedEvent create(FeedbackMark mark) {
        return INSTANCES.get(mark);
    }

}
