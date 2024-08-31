package ru.yandex.alice.kronstadt.core;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertNotEquals;

class VcsUtilsTest {

    @Test
    void name() {
        var vcsUtils = new VcsUtils();

        assertNotEquals("", vcsUtils.getVersion());
        assertNotEquals("", vcsUtils.getBranch());
    }
}
