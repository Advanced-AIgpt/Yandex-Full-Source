package ru.yandex.alice.paskill.dialogovo.service;

import java.util.Set;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Surface;

public interface SurfaceChecker {
    boolean isSkillSupported(ClientInfo clientInfo, Set<Surface> surfaces);

}

