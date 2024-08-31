package ru.yandex.alice.kronstadt.server

import org.apache.logging.log4j.MarkerManager
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace

internal val SELECTED_SCENE_SETRACE_MARKER =
    MarkerManager.getMarker("Kronstadt scene").addParents(Setrace.SETRACE_TAG_MARKER_PARENT)
