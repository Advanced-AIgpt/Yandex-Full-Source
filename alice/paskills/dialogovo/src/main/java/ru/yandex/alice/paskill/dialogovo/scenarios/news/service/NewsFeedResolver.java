package ru.yandex.alice.paskill.dialogovo.scenarios.news.service;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

@Component
public class NewsFeedResolver {
    private static final Logger logger = LogManager.getLogger();

    public Optional<NewsFeed> findDefaultFeed(NewsSkillInfo skillInfo) {

        Optional<NewsFeed> defaultFeedO = skillInfo.getDefaultFeed();
        if (defaultFeedO.isEmpty()) {
            logger.warn("Unable to detect default news feed by skill [{}]", skillInfo.getId());
            return Optional.empty();
        }

        NewsFeed defaultFeed = defaultFeedO.get();
        if (!defaultFeed.isEnabled()) {
            logger.info("Default news feed [{}] from skill [{}] is disabled",
                    skillInfo.getId(), defaultFeed.getId());
            return Optional.empty();
        }

        return Optional.of(defaultFeed);
    }

    public Optional<NewsFeed> findFeedById(NewsSkillInfo skillInfo, String feedId) {
        return skillInfo.getFeeds()
                .stream()
                .filter(feed -> feed.getId().equals(feedId))
                .findFirst();
    }
}
