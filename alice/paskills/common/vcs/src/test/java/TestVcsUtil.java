package ru.yandex.alice.paskills.common.vcs;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;


class TestVcsUtil {

    @Test
    void name() {
        String vcs = VcsUtils.INSTANCE.getVersion();
        System.out.println(vcs);
        assertNotNull(vcs);
        // this may fail when run built and run in IDEA, it's ok, manifest is created by ya make
        assertNotEquals("-1", vcs);
    }
}
