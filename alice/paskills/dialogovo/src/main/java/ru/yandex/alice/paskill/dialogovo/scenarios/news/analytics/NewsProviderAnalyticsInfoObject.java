package ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics;

import javax.annotation.Nonnull;

import lombok.Getter;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TNewsProvider;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject;

@Getter
public class NewsProviderAnalyticsInfoObject extends AnalyticsInfoObject {
    private final String providerName;
    private final String feedId;

    public NewsProviderAnalyticsInfoObject(String id, String providerName, String feedId) {
        super(id, "external_skill.news.source", "Источник новостей");
        this.providerName = providerName;
        this.feedId = feedId;
    }

    @Nonnull
    @Override
    public TObject.Builder fillProtoField(@Nonnull TObject.Builder protoBuilder) {
        var payload = TNewsProvider.newBuilder()
                .setName(providerName)
                .setFeedId(feedId);
        return protoBuilder.setNewsProvider(payload);
    }
}
