package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.util.concurrent.CompletableFuture;

public interface SkillRequestLogPersistent {
    CompletableFuture<Void> save(LogRecord record);
}
