package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;
import java.util.Random;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsGetDetailsDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.ScenarioResponseResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsContentsResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsProviderSuggestService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;

@Component
public class NewsGetDetailsCallbackProcessor extends BaseFlashBriefingCallbackProcessor {

    private final NewsContentsResolver newsContentsResolver;
    private final ScenarioResponseResolver scenarioResponseResolver;

    @SuppressWarnings("ParameterNumber")
    protected NewsGetDetailsCallbackProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            ScenarioResponseResolver scenarioResponseResolver,
            NewsProviderSuggestService newsProviderSuggestService,
            AppMetricaEventSender appMetricaEventSender) {
        super(newsCommitService, skillProvider, newsFeedResolver, newsSkillStateMaintainer, newsProviderSuggestService,
                appMetricaEventSender);
        this.newsContentsResolver = newsContentsResolver;
        this.scenarioResponseResolver = scenarioResponseResolver;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return request.getInput().isCallback(NewsGetDetailsDirective.class);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_NEWS_GET_DETAILS;
    }

    @Override
    protected NewsPayload getNewsPayload(MegaMindRequest<DialogovoState> request) {
        NewsGetDetailsDirective payload = request.getInput().getDirective(NewsGetDetailsDirective.class);

        return NewsPayload.from(payload);
    }

    @Override
    protected Optional<NewsContent> getNewsContent(List<NewsContent> topNewsByFeed, Optional<String> contentIdO,
                                                   int depth) {
        return newsContentsResolver.findById(topNewsByFeed, contentIdO.get());
    }

    @Override
    protected Optional<BaseRunResponse> onFeedByIdNotFound(MegaMindRequest<DialogovoState> request) {
        return Optional.of(createIrrelevantResult(request));
    }

    @Override
    @SuppressWarnings("ParameterNumber")
    protected ScenarioResponseBody<DialogovoState> onProcessFinished(
            NewsSkillInfo skillInfo,
            Context context,
            Random random,
            NewsFeed newsFeed,
            NewsContent newsContent,
            DialogovoState dialogovoState,
            List<NewsContent> topNewsByFeed,
            MegaMindRequest<DialogovoState> request,
            List<NewsSkillInfo> proversSuggests) {
        return scenarioResponseResolver.get(skillInfo.getFlashBriefingType())
                .onGetDetails(dialogovoState, context, random, new NewsArticle(skillInfo, newsFeed, newsContent,
                        false, false), request.getClientInfo());
    }

    @Override
    protected String getMetricaEventName() {
        return "news_details";
    }
}
