package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;
import java.util.Random;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.NewsProviderAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsReadPrevDirective;
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
public class ReadPrevNewsCallbackProcessor extends BaseFlashBriefingCallbackProcessor {

    private final NewsContentsResolver newsContentsResolver;
    private final ScenarioResponseResolver scenarioResponseResolver;

    @SuppressWarnings("ParameterNumber")
    protected ReadPrevNewsCallbackProcessor(
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
        return request.getInput().isCallback(NewsReadPrevDirective.class);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_NEWS_READ_PREV;
    }

    @Override
    protected NewsPayload getNewsPayload(MegaMindRequest<DialogovoState> request) {
        NewsReadPrevDirective payload = request.getInput().getDirective(NewsReadPrevDirective.class);

        return NewsPayload.from(payload);
    }

    @Override
    protected Optional<NewsContent> getNewsContent(List<NewsContent> topNewsByFeed, Optional<String> contentIdO,
                                                   int depth) {
        return newsContentsResolver.findPrevOne(topNewsByFeed, contentIdO.get());
    }

    @Override
    protected Optional<BaseRunResponse> onFeedByIdNotFound(MegaMindRequest<DialogovoState> request) {
        return Optional.of(createIrrelevantResult(request));
    }

    @Override
    protected Optional<BaseRunResponse> onNewsContentEmpty(DialogovoState dialogovoState, Context context,
                                                           Random random, NewsFeed newsFeed, NewsSkillInfo skillInfo,
                                                           MegaMindRequest<DialogovoState> request,
                                                           List<NewsSkillInfo> proversSuggest) {
        return getRepeatSuggestResponse(dialogovoState, context, newsFeed, skillInfo, random);
    }

    private Optional<BaseRunResponse> getRepeatSuggestResponse(DialogovoState dialogovoState, Context context,
                                                               NewsFeed newsFeed, NewsSkillInfo skillInfo,
                                                               Random random) {
        return Optional.of(new RunOnlyResponse<>(
                scenarioResponseResolver.get(skillInfo.getFlashBriefingType())
                        .repeatError(
                                dialogovoState,
                                context,
                                newsFeed,
                                skillInfo,
                                random))
        );
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
            List<NewsSkillInfo> proversSuggest) {
        context.getAnalytics().addAction(AnalyticsInfoActionsFactory.createActivateAction(skillInfo));
        context.getAnalytics().addObject(new NewsProviderAnalyticsInfoObject(
                skillInfo.getId(),
                skillInfo.getName(),
                newsFeed.getId()));
        return scenarioResponseResolver.get(skillInfo.getFlashBriefingType())
                .onReadContent(dialogovoState, context,
                        Optional.empty(), new NewsArticle(
                                skillInfo, newsFeed, newsContent, false,
                                newsContentsResolver.findNextOneFrom(topNewsByFeed, newsContent.getId()).isPresent()),
                        request, proversSuggest);
    }
}
