package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;


import java.util.Optional;
import java.util.function.BiPredicate;

import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

public class MostRecentContentReadPredicate implements BiPredicate<NewsSkillInfo, DialogovoState.NewsState> {

    public static final MostRecentContentReadPredicate INSTANCE = new MostRecentContentReadPredicate();

    private MostRecentContentReadPredicate() {

    }

    @Override
    // checks if there is new fresh content by provider not yet viewed by user based on newsState
    public boolean test(NewsSkillInfo newsSkillInfo, DialogovoState.NewsState newsState) {
        if (newsSkillInfo.getDefaultFeed().isEmpty()) {
            return false;
        }

        NewsFeed newsFeed = newsSkillInfo.getDefaultFeed().get();

        Optional<String> lastNewsReadBySkillFeedO = newsState.getLastNewsReadBySkillFeed(
                newsSkillInfo.getId(),
                newsFeed.getId());

        if (lastNewsReadBySkillFeedO.isEmpty()) {
            return true;
        }

        String lastNewsReadBySkillFeedId = lastNewsReadBySkillFeedO.get();

        Optional<NewsContent> mostFreshO = newsFeed.getTopContents().stream().findFirst();

        if (mostFreshO.isEmpty()) {
            return false;
        }

        NewsContent mostFresh = mostFreshO.get();

        if (mostFresh.getId().equals(lastNewsReadBySkillFeedId)) {
            return false;
        }

        return true;
    }
}
