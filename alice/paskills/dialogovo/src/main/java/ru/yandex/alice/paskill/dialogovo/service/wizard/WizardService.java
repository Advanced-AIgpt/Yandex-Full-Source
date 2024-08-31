package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;

public interface WizardService {
    WizardResponse requestWizard(Optional<String> utterance,
                                 Optional<String> grammarBase64);

    CompletableFuture<WizardResponse> requestWizardAsync(Optional<String> utterance,
                                                         Optional<String> grammarBase64);
}
