package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.util.concurrent.CompletableFuture;

public class InMemorySkillRequestLogPersistent implements SkillRequestLogPersistent {
    @Override
    public CompletableFuture<Void> save(LogRecord record) {
        return CompletableFuture.completedFuture(null);
    }
}
