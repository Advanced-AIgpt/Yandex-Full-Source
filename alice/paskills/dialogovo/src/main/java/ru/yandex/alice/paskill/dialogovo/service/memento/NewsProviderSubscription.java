package ru.yandex.alice.paskill.dialogovo.service.memento;

import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

@Getter
public class NewsProviderSubscription {
    private final String newsProviderSlug;
    // topic in news.feed terminology - not uses now
    private final Optional<String> newsRubric;
    private final boolean defaultValue;

    public NewsProviderSubscription(String newsProviderSlug, Optional<String> newsRubric) {
        this.newsProviderSlug = newsProviderSlug;
        this.newsRubric = newsRubric;
        this.defaultValue = false;
    }

    @JsonCreator
    public NewsProviderSubscription(
            @JsonProperty("newsProviderSlug") String newsProviderSlug,
            @JsonProperty("newsRubric") Optional<String> newsRubric,
            @JsonProperty("defaultValue") boolean defaultValue) {
        this.newsProviderSlug = newsProviderSlug;
        this.newsRubric = newsRubric;
        this.defaultValue = defaultValue;
    }
}
