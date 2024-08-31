package ru.yandex.alice.paskill.dialogovo.service;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;

public interface SocialService {
    CompletableFuture<Optional<String>> getSocialTokenAsync(String userId, String oauthAppName);

    Optional<String> getSocialToken(String userId, String oauthAppName);
}
