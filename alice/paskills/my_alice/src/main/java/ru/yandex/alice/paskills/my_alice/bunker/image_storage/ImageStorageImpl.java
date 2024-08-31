package ru.yandex.alice.paskills.my_alice.bunker.image_storage;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.MediaType;
import org.springframework.lang.Nullable;

import ru.yandex.alice.paskills.my_alice.bunker.client.BunkerClient;
import ru.yandex.alice.paskills.my_alice.bunker.client.NodeInfo;

public class ImageStorageImpl implements ImageStorage {
    private static final MediaType IMAGES = new MediaType("image", "*+xml");
    private static final String URL_PARAM_NAME = "avatar-href";

    private static final Logger logger = LogManager.getLogger();

    private final BunkerClient bunker;
    private final String projectRoot;
    private Map<String, String> data = Map.of();
    private boolean isReady = false;

    ImageStorageImpl(BunkerClient bunker, String projectRoot, boolean loadDataAtInit) {
        this.bunker = bunker;
        this.projectRoot = projectRoot;

        if (loadDataAtInit) {
            loadData();
        }
    }

    ImageStorageImpl(BunkerClient bunker, String projectRoot, Map<String, NodeInfo> bunkerTree) {
        this.bunker = bunker;
        this.projectRoot = projectRoot;

        applyBunkerTree(bunkerTree);

        this.isReady = true;
    }

    @Override
    public Optional<String> getRealUrl(@Nullable String url) {
        return Optional.ofNullable(url)
                .map(s -> s.startsWith("bunker:/") ? data.get(s) : s);
    }

    @Override
    public boolean isReady() {
        return isReady;
    }

    private void loadData() {
        logger.debug("Refreshing Bunker images storage data. projectRoot={}", projectRoot);
        try {
            applyBunkerTree(bunker.tree(projectRoot));

            if (!this.isReady) {
                this.isReady = true;
            }
        } catch (Exception e) {
            logger.warn("Failed to load images mapping data", e);
        }
    }

    private void applyBunkerTree(Map<String, NodeInfo> nodeMap) {
        Map<String, String> newData = new HashMap<>();

        for (var entry : nodeMap.entrySet()) {
            var node = entry.getValue();
            var mediaType = node.getMediaType();

            if (mediaType == null || !IMAGES.isCompatibleWith(mediaType)) {
                continue;
            }

            var url = mediaType.getParameter(URL_PARAM_NAME);
            if (url == null) {
                continue;
            }

            url = url.replaceFirst("^\"", "").replaceAll("\\\"", "\"").replaceFirst("\"$", "");
            if (url.length() > 0) {
                newData.put("bunker:" + node.getPath(), url);
            }
        }

        this.data = newData;
    }
}
