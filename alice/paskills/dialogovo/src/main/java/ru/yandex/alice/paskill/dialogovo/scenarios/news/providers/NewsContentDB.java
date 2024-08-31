package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class NewsContentDB {
    private final String id;
    private final String feedId;
    private final String uid;
    private final Instant pubDate;
    private final String title;
    @Nullable
    private final String streamUrl;
    private final String mainText;
    @Nullable
    private final String soundId;
    @Nullable
    private final String imageUrl;
    @Nullable
    private final String detailsUrl;
    @Nullable
    private final String detailsText;
}
