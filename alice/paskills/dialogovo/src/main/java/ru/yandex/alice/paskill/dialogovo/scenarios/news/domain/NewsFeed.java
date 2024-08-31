package ru.yandex.alice.paskill.dialogovo.scenarios.news.domain;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class NewsFeed {
    private final String id;
    private final String skillId;
    @Nullable
    private final String preamble;
    private final String name;
    private final String topic;
    private final boolean enabled;
    private final int depth;
    private final List<NewsContent> topContents;

    public Optional<String> getPreamble() {
        return Optional.ofNullable(preamble);
    }
}
