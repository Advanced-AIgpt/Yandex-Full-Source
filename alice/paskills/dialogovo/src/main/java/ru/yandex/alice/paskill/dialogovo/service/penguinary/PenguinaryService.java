package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import java.util.concurrent.CompletableFuture;

public interface PenguinaryService {
    PenguinaryResult findSkillsByUtterance(String utterance);

    CompletableFuture<PenguinaryResult> findSkillsByUtteranceAsync(String utterance);
}
