package ru.yandex.alice.paskill.dialogovo.scenarios.news.service;

import java.util.List;
import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;

@Component
public class NewsContentsResolver {

    public Optional<NewsContent> findNextOne(List<NewsContent> topNewsByFeed, Optional<String> lastNewsReadBySkillFeed,
                                             int depth) {
        if (topNewsByFeed.isEmpty()) {
            return Optional.empty();
        }

        if (lastNewsReadBySkillFeed.isPresent()) {
            return findNextOneOnContinue(topNewsByFeed, lastNewsReadBySkillFeed.get(), depth);
        } else {
            return getCurrentTop(topNewsByFeed, depth);
        }
    }

    private Optional<NewsContent> findNextOneOnContinue(List<NewsContent> topNewsByFeed,
                                                        String lastNewsReadBySkillFeed, int depth) {
        if (findById(topNewsByFeed, lastNewsReadBySkillFeed).isPresent()) {
            return findNextOneFrom(topNewsByFeed, lastNewsReadBySkillFeed);
        } else {
            return getCurrentTop(topNewsByFeed, depth);
        }
    }

    private Optional<NewsContent> getCurrentTop(List<NewsContent> topNewsByFeed, int depth) {
        return topNewsByFeed.stream()
                .skip(Math.min(depth - 1, topNewsByFeed.size() - 1))
                .findFirst();
    }

    public Optional<NewsContent> findNextOneFrom(List<NewsContent> topNewsByFeed, String fromContentId) {
        Optional<NewsContent> candidate = Optional.empty();
        for (NewsContent newsContent : topNewsByFeed) {
            if (newsContent.getId().equals(fromContentId)) {
                return candidate;
            }

            candidate = Optional.of(newsContent);
        }

        return candidate;
    }

    public Optional<NewsContent> findPrevOne(List<NewsContent> topNewsByFeed, String fromContentId) {
        if (findById(topNewsByFeed, fromContentId).isEmpty()) {
            return Optional.empty();
        }
        Optional<NewsContent> candidate = Optional.empty();
        for (int currNews = topNewsByFeed.size() - 1; currNews >= 0; currNews--) {
            var newsContent = topNewsByFeed.get(currNews);
            if (newsContent.getId().equals(fromContentId)) {
                return candidate;
            }

            candidate = Optional.of(newsContent);
        }

        return candidate;
    }

    public Optional<NewsContent> findById(List<NewsContent> topNewsByFeed, String contentId) {
        return topNewsByFeed
                .stream()
                .filter(content -> content.getId().equals(contentId))
                .findFirst();
    }
}
