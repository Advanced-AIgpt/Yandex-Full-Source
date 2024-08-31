package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides;

import java.util.List;
import java.util.Optional;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;

public class ArticleOverrides {

    private final List<ArticleOverride> articleOverrides;

    public ArticleOverrides(List<ArticleOverride> articleOverrides) {
        this.articleOverrides = articleOverrides;
    }

    public Optional<ArticleOverride> getOverrideO(NewsArticle article) {
        return articleOverrides.stream().filter(override -> override.canOverride(article)).findFirst();
    }
}
