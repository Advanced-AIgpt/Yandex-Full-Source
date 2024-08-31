package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Random;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective;
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsSendDetailsLinkDirective;
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

import static org.apache.commons.lang.StringUtils.abbreviate;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.capitalizeFirst;

@Component
public class NewsSendDetailsLinkCallbackProcessor extends BaseFlashBriefingCallbackProcessor {

    private static final int MAX_PUSH_BODY_WIDTH = 30;
    private final NewsContentsResolver newsContentsResolver;
    private final ScenarioResponseResolver scenarioResponseResolver;
    private final RequestContext requestContext;

    private static final Logger logger = LogManager.getLogger();

    @SuppressWarnings("ParameterNumber")
    protected NewsSendDetailsLinkCallbackProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            ScenarioResponseResolver scenarioResponseResolver,
            NewsProviderSuggestService newsProviderSuggestService,
            RequestContext requestContext,
            AppMetricaEventSender appMetricaEventSender
    ) {
        super(newsCommitService, skillProvider, newsFeedResolver, newsSkillStateMaintainer, newsProviderSuggestService,
                appMetricaEventSender);
        this.newsContentsResolver = newsContentsResolver;
        this.scenarioResponseResolver = scenarioResponseResolver;
        this.requestContext = requestContext;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return request.getInput().isCallback(NewsSendDetailsLinkDirective.class);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_NEWS_SEND_DETAILS_LINK;
    }

    @Override
    protected NewsPayload getNewsPayload(MegaMindRequest<DialogovoState> request) {
        NewsSendDetailsLinkDirective payload = request.getInput().getDirective(NewsSendDetailsLinkDirective.class);

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
            MegaMindRequest<DialogovoState> request, List<NewsSkillInfo> proversSuggests) {

        // currentUserId для станции всегда есть
        List<ServerDirective> directives = new ArrayList<>(1);
        if (newsContent.getDetailsUrl().isPresent()) {
            logger.info("send news details push for user {}", requestContext.getCurrentUserId());
            directives.add(new SendPushMessageDirective(
                    skillInfo.getName(),
                    abbreviate(capitalizeFirst(newsContent.getTitle()), MAX_PUSH_BODY_WIDTH),
                    URI.create(newsContent.getDetailsUrl().get()),
                    "alice_flash_briefing_send_news_details", // TODO change to common alice push id
                    skillInfo.getId() + "-flash_briefing_content-request",
                    SendPushMessageDirective.ALICE_DEFAULT_DEVICE_ID,
                    List.of(SendPushMessageDirective.AppType.SEARCH_APP),
                    skillInfo.getName(),
                    "Открыть"
            ));
        }

        return scenarioResponseResolver.get(skillInfo.getFlashBriefingType())
                .onSendDetailsLink(dialogovoState, context, random, new NewsArticle(skillInfo, newsFeed, newsContent,
                        false, false), request.getClientInfo(), directives);
    }

    @Override
    protected String getMetricaEventName() {
        return "news_details_send_link";
    }
}
