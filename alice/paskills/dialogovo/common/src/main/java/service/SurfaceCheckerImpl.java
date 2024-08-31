package ru.yandex.alice.paskill.dialogovo.service;

import java.util.Set;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Surface;

@Component
class SurfaceCheckerImpl implements SurfaceChecker {
    @Override
    public boolean isSkillSupported(ClientInfo clientInfo, Set<Surface> surfaces) {
        if (clientInfo.isDevConsole() || surfaces.isEmpty() || clientInfo.isFloyd()) {
            return true;
        }

        if (clientInfo.isNavigatorOrMaps() && !surfaces.contains(Surface.NAVIGATOR)) {
            return false;
        }

        // хак чтобы включить навыки с экраном для centaur
        // TODO: выпилить когда centaur запустится и будет уже в дев-консоли публичной поверхностью
        if (clientInfo.isCentaur() && surfaces.contains(Surface.MOBILE)) {
            return true;
        }

        if (clientInfo.isYaSmartDevice() && !surfaces.contains(Surface.QUASAR)) {
            return false;
        }

        if (clientInfo.isElariWatch() && !surfaces.contains(Surface.WATCH)) {
            return false;
        }

        return !clientInfo.isYaAuto() || surfaces.contains(Surface.AUTO);
    }

}
