package ru.yandex.alice.paskill.dialogovo.scenarios.news.domain;

import java.time.Instant;
import java.util.Optional;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class NewsContent {
    private final String id;
    private final String feedId;
    private final String uid;
    private final Instant pubDate;
    private final String title;
    @Nullable
    private final String streamUrl;
    @Nullable
    private final String mainText;
    @Nullable
    private final String soundId;
    @Nullable
    private final String imageUrl;
    @Nullable
    private final String detailsUrl;
    @Nullable
    private final String detailsText;

    public Optional<String> getStreamUrl() {
        return Optional.ofNullable(streamUrl);
    }

    public Optional<String> getImageUrl() {
        return Optional.ofNullable(imageUrl);
    }

    public Optional<String> getDetailsUrl() {
        return Optional.ofNullable(detailsUrl);
    }

    public Optional<String> getSoundId() {
        return Optional.ofNullable(soundId);
    }

    public Optional<String> getMainText() {
        return Optional.ofNullable(mainText);
    }

    public Optional<String> getDetailsText() {
        return Optional.ofNullable(detailsText);
    }
}
