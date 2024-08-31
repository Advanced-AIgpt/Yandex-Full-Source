package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;


import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.function.BiPredicate;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.NewsConstants;

public class HasFreshContentProviderPredicate implements BiPredicate<Instant, NewsSkillInfo> {

    public static final HasFreshContentProviderPredicate INSTANCE = new HasFreshContentProviderPredicate();
    private static final Duration MAX_ALLOWED_CONTENT_STILLNESS_FOR_PROVIDER_LIVENESS_DEFAULT = Duration.ofDays(3);
    // slug to liveness override
    // todo: move to DB
    private static final Map<String, Duration> MAX_ALLOWED_CONTENT_STILLNESS_FOR_PROVIDER_LIVENESS_OVERRIDES =
            Map.of(NewsConstants.KIDS_NEWS_PROVIDER_SKILL_SLUG, Duration.ofDays(5));

    private HasFreshContentProviderPredicate() {

    }

    @Override
    public boolean test(Instant now, NewsSkillInfo newsSkillInfo) {
        return newsSkillInfo
                .getFeeds()
                .stream()
                .allMatch(feed -> {
                    List<NewsContent> topContents = feed.getTopContents();

                    if (topContents.isEmpty()) {
                        return false;
                    }

                    // correct order of news content per feed provided by newsContentsDao
                    NewsContent mostFresh = topContents.get(0);

                    return (now.toEpochMilli() - mostFresh.getPubDate().toEpochMilli())
                            < getMaxAllowedContentStillnessForProviderLiveness(newsSkillInfo).toMillis();
                });
    }

    private Duration getMaxAllowedContentStillnessForProviderLiveness(NewsSkillInfo newsSkillInfo) {
        return MAX_ALLOWED_CONTENT_STILLNESS_FOR_PROVIDER_LIVENESS_OVERRIDES.getOrDefault(newsSkillInfo.getSlug(),
                MAX_ALLOWED_CONTENT_STILLNESS_FOR_PROVIDER_LIVENESS_DEFAULT);
    }
}
