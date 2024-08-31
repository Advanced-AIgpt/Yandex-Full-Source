package ru.yandex.alice.paskill.dialogovo.service.ner;

import java.util.concurrent.CompletableFuture;

import javax.annotation.Nullable;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;

public interface NerService {
    CompletableFuture<Nlu> getNluAsync(@Nullable String utterance, String skillId);

    Nlu getNlu(@Nullable String utterance, String skillId);
}
