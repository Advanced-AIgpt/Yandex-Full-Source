package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import java.util.Collections;
import java.util.List;

import lombok.Data;

@Data
public class PenguinaryResult {

    public static final PenguinaryResult EMPTY = new PenguinaryResult(Collections.emptyList());

    // list sorted by distance ascending
    private final List<Candidate> candidates;

    @Data
    public static class Candidate {
        private final double distance;
        private final String documentId;
    }
}
