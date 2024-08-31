package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class NewsFeedDB {
    private final String id;
    private final String skillId;
    @Nullable
    private final String preamble;
    private final String name;
    private final String topic;
    private final boolean enabled;
}
