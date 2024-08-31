package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides;

import java.util.Optional;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;

public interface ArticleOverride {
    boolean canOverride(NewsArticle article);

    default Optional<String> overrideTitle(NewsArticle article, ClientInfo clientInfo) {
        return Optional.of(article.getContent().getTitle());
    }

    default Optional<Optional<String>> overrideMainText(NewsArticle article, ClientInfo clientInfo) {
        return Optional.of(article.getContent().getMainText());
    }
}
