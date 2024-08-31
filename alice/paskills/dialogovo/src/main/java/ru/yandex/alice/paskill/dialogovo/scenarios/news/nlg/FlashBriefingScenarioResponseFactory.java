package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg;

import java.util.List;
import java.util.Optional;
import java.util.Random;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

public interface FlashBriefingScenarioResponseFactory {
    ScenarioResponseBody<DialogovoState> onReadContent(DialogovoState dialogovoState,
                                                       Context context,
                                                       Optional<NewsSkillInfo> nextProviderPostrollO,
                                                       NewsArticle newsArticle,
                                                       MegaMindRequest<DialogovoState> request,
                                                       List<NewsSkillInfo> providersSuggest);

    ScenarioResponseBody<DialogovoState> onGetDetails(DialogovoState dialogovoState,
                                                      Context context,
                                                      Random random,
                                                      NewsArticle newsArticle,
                                                      ClientInfo clientInfo);

    ScenarioResponseBody<DialogovoState> onSendDetailsLink(DialogovoState dialogovoState,
                                                           Context context,
                                                           Random random,
                                                           NewsArticle newsArticle,
                                                           ClientInfo clientInfo,
                                           List<ServerDirective> serverDirectives);

    ScenarioResponseBody<DialogovoState> onEmptyContentPerFeed(DialogovoState dialogovoState,
                                                               Context context,
                                                               Random random,
                                                               NewsFeed newsFeed,
                                                               NewsSkillInfo skillInfo,
                                                               ClientInfo clientInfo,
                                                               List<NewsSkillInfo> providersSuggest);

    ScenarioResponseBody<DialogovoState> repeatError(DialogovoState dialogovoState, Context context, NewsFeed newsFeed,
                                                     NewsSkillInfo skillInfo, Random random);
}
