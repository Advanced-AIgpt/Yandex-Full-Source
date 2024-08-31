package ru.yandex.alice.vault;

public record VersionResponse(
        Status status,
        Version version
) {
}
