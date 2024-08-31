package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import lombok.Data;

@Data
class PenguinaryResponse {
    @Nullable
    private final double[] distances;
    @Nullable
    private final List<String> intents;

    public double[] getDistances() {
        return distances != null ? distances : new double[0];
    }

    public List<String> getIntents() {
        return intents != null ? intents : Collections.emptyList();
    }
}
