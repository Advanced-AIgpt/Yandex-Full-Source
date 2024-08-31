package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides;


import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Locale;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;

public class RadioNewsArticleOverride implements ArticleOverride {
    protected static final Logger logger = LogManager.getLogger();

    private static final DateTimeFormatter DATE_TIME_FORMATTER = DateTimeFormatter.ofPattern("dd MMMM yyyy HH:mm");

    private final Phrases phrases;

    public RadioNewsArticleOverride(Phrases phrases) {
        this.phrases = phrases;
    }

    @Override
    // all radionews overrides now are the same
    public boolean canOverride(NewsArticle article) {
        return article.getContent().getStreamUrl().isPresent();
    }

    @Override
    public Optional<String> overrideTitle(NewsArticle article, ClientInfo clientInfo) {
        try {
            return Optional.of(phrases.get("news.content.tv.text", List.of(
                    article.getSkill().getName(),
                    DATE_TIME_FORMATTER.withLocale(Locale.forLanguageTag(clientInfo.getLang())).format(
                            LocalDateTime.ofInstant(article.getContent().getPubDate(),
                                    ZoneId.of(clientInfo.getTimezone()))
                    ))));
        } catch (Exception e) {
            logger.warn("Cannot override article title with clientInfo=[" + clientInfo + "] " +
                    "article=[" + article + "]", e);
            return Optional.empty();
        }
    }

    @Override
    public Optional<Optional<String>> overrideMainText(NewsArticle article, ClientInfo clientInfo) {
        return Optional.of(Optional.empty());
    }
}
