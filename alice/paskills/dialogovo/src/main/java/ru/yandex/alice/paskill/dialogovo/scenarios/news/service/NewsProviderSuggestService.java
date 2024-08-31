package ru.yandex.alice.paskill.dialogovo.scenarios.news.service;

import java.time.Instant;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.FlashBriefingType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.NewsConstants;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.HasFreshContentProviderPredicate;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.MostRecentContentReadPredicate;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;

@Component
public class NewsProviderSuggestService {
    private static final Logger logger = LogManager.getLogger();
    private static final int MAX_POSTROLL_SEQ_LENGTH = 5;

    private final NewsContentsResolver newsContentsResolver;
    private final NewsSkillProvider newsSkillProvider;

    public NewsProviderSuggestService(
            NewsContentsResolver newsContentsResolver,
            NewsSkillProvider newsSkillProvider) {
        this.newsContentsResolver = newsContentsResolver;
        this.newsSkillProvider = newsSkillProvider;
    }

    public Optional<NewsSkillInfo> postrollAfter(
            NewsSkillInfo afterSkill, DialogovoState.NewsState newsState, Random random, Instant now) {
        return postrollAfter(afterSkill, newsState, random, now, true);
    }

    public List<NewsSkillInfo> suggest(DialogovoState.NewsState newsState, Random random, Instant now,
                                       int maxProviders) {
        List<NewsSkillInfo> providersToSuggest = newsSkillProvider.findAllActive()
                .stream()
                .filter(skill -> skill.getFlashBriefingType() == FlashBriefingType.RADIONEWS)
                .filter(provider -> !newsState.getPostrolledProviders().contains(provider.getId()))
                .filter(provider -> HasFreshContentProviderPredicate.INSTANCE.test(now, provider))
                .filter(provider -> MostRecentContentReadPredicate.INSTANCE.test(provider, newsState))
                // do not postroll kids news to adults
                .filter(provider -> !provider.getSlug().equals(NewsConstants.KIDS_NEWS_PROVIDER_SKILL_SLUG))
                .limit(maxProviders)
                .collect(Collectors.toList());

        if (providersToSuggest.isEmpty()) {
            logger.info("Not suggesting cause none no appropriate providers found");
            return providersToSuggest;
        }

        Collections.shuffle(providersToSuggest, random);

        logger.info("Prepared suggest for news providers; Provider names=[{}]",
                providersToSuggest.stream().map(NewsSkillInfo::getName).collect(Collectors.toList()));

        return providersToSuggest;
    }

    public Optional<NewsSkillInfo> postrollAfter(NewsSkillInfo afterSkill, DialogovoState.NewsState newsState,
                                                 Random random,
                                                 Instant now, boolean withRespectOfMaxLen) {
        if (withRespectOfMaxLen && newsState.getPostrolledProviders().size() >= MAX_POSTROLL_SEQ_LENGTH) {
            logger.info("Not postrolling next provider cause postroll seq exceed max length in time window");
            return Optional.empty();
        }

        List<NewsSkillInfo> providersToSuggest = newsSkillProvider.findAllActive()
                .stream()
                .filter(skill -> skill.getFlashBriefingType() == FlashBriefingType.RADIONEWS)
                .filter(provider -> !newsState.getPostrolledProviders().contains(provider.getId()))
                .filter(skill -> !afterSkill.getId().equals(skill.getId()))
                .filter(provider -> HasFreshContentProviderPredicate.INSTANCE.test(now, provider))
                // do not postroll kids news to adults
                .filter(provider -> !provider.getSlug().equals(NewsConstants.KIDS_NEWS_PROVIDER_SKILL_SLUG))
                .collect(Collectors.toList());

        if (providersToSuggest.isEmpty()) {
            logger.info("Not postrolling cause none non postrolled providers left in time window");
            return Optional.empty();
        }

        Collections.shuffle(providersToSuggest, random);

        Optional<NewsSkillInfo> nextO = providersToSuggest.stream()
                .filter(provider -> provider.getDefaultFeed().isPresent())
                .filter(provider -> {
                    NewsFeed newsFeed = provider.getDefaultFeed().get();
                    return newsContentsResolver.findNextOne(
                            newsFeed.getTopContents(),
                            newsState.getLastNewsReadBySkillFeed(provider.getId(), newsFeed.getId()),
                            newsFeed.getDepth()
                    ).isPresent();
                })
                .findFirst();

        if (nextO.isEmpty()) {
            logger.info("Not postrolling next provider cause no fresh content in other providers");
        } else {
            logger.info("Prepared postroll news next provider id=[{}] after id=[{}]",
                    nextO.map(NewsSkillInfo::getId), afterSkill.getId());
        }

        return nextO;
    }
}
