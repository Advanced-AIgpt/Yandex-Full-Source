package ru.yandex.alice.paskills.my_alice.bunker.main_promo;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.util.Strings;
import org.springframework.lang.Nullable;

import ru.yandex.alice.paskills.my_alice.bunker.client.BunkerClient;
import ru.yandex.alice.paskills.my_alice.bunker.image_storage.ImageStorage;
import ru.yandex.alice.paskills.my_alice.layout.Card;
import ru.yandex.alice.paskills.my_alice.layout.PageLayout;

public class MainPromoImpl implements MainPromo {
    private static final Logger logger = LogManager.getLogger();

    private final BunkerClient bunker;
    private final String projectRoot;
    private final ImageStorage imageStorage;
    private final Random random;
    @Nullable
    private Data data = null;
    private boolean isReady = false;

    MainPromoImpl(BunkerClient bunker, String projectRoot, ImageStorage imageStorage, boolean loadDataAtInit) {
        this(bunker, projectRoot, imageStorage, new Random(), loadDataAtInit);
    }

    MainPromoImpl(
            BunkerClient bunker,
            String projectRoot,
            ImageStorage imageStorage,
            Random random,
            boolean loadDataAtInit
    ) {
        this.bunker = bunker;
        this.projectRoot = projectRoot;
        this.imageStorage = imageStorage;
        this.random = random;

        if (loadDataAtInit) {
            loadData();
        }
    }

    protected MainPromoImpl(
            BunkerClient bunker,
            String projectRoot,
            ImageStorage imageStorage,
            Random random,
            Data data
    ) {
        this.bunker = bunker;
        this.projectRoot = projectRoot;
        this.imageStorage = imageStorage;
        this.random = random;
        this.data = data;
    }

    @Override
    public Optional<PageLayout.Card> getCard(boolean isAuthorized) {
        if (data == null) {
            return Optional.empty();
        }

        if (data.getShowProbability() <= random.nextDouble()) {
            return Optional.empty();
        }

        var blockO = chooseBlock(data, isAuthorized);
        if (blockO.isEmpty()) {
            return Optional.empty();
        }

        var cardO = chooseCard(blockO.get());
        if (cardO.isEmpty()) {
            return Optional.empty();
        }

        Map<String, String> metrikaParams = new HashMap<>();
        if (Strings.isNotBlank(blockO.get().getId())) {
            metrikaParams.put("mainPromo_blockId", blockO.get().getId());

            if (Strings.isNotBlank(cardO.get().getId())) {
                metrikaParams.put("mainPromo_cardId", cardO.get().getId());
            }
        }

        return Optional.of(new ru.yandex.alice.paskills.my_alice.layout.Card(
                        Card.Kind.SCENARIO,
                        cardO.get().getButton(),
                        cardO.get().getCaption(),
                        cardO.get().getText(),
                        imageStorage.getRealUrl(cardO.get().getImage()).orElse(null),
                        cardO.get().getColor(),
                        metrikaParams
                )
        );
    }

    @Override
    public boolean isReady() {
        return isReady;
    }

    private void loadData() {
        logger.debug("Refreshing Bunker main promo data. node={}/main-promo", projectRoot);
        try {
            var resultO = bunker.get(projectRoot + "/main-promo", Data.class);
            if (resultO.isEmpty() || resultO.get().getContent() == null) {
                logger.warn("Failed to load main promo data from Bunker: result is empty");
                return;
            }

            this.data = resultO.get().getContent();

            if (!this.isReady) {
                this.isReady = true;
            }
        } catch (Exception e) {
            logger.warn("Failed to load main promo data from Bunker", e);
        }
    }

    private Optional<Data.Block> chooseBlock(Data data, Boolean isAuthorized) {
        String authRule = isAuthorized ? "required" : "deny";

        var filteredViaAuthStatus = data.getBlocks().stream()
                .filter(block -> "everybody".equals(block.getAuth()) || authRule.equals(block.getAuth()))
                .collect(Collectors.toList());

        double seek = random.nextDouble() *
                filteredViaAuthStatus.stream().mapToDouble(Data.Block::getWeight).map(Math::abs).sum();

        for (Data.Block item : filteredViaAuthStatus) {
            if (item.getWeight() == 0.0) {
                continue;
            }

            seek -= item.getWeight();
            if (seek <= 0.0d) {
                return Optional.of(item);
            }
        }

        return Optional.empty();
    }

    Optional<Data.Card> chooseCard(Data.Block block) {
        double seek = random.nextDouble() * block.getCards().stream().mapToDouble(Data.Card::getWeight).sum();

        for (Data.Card item : block.getCards()) {
            if (item.getWeight() == 0.0) {
                continue;
            }

            seek -= item.getWeight();
            if (seek <= 0.0d) {
                return Optional.of(item);
            }
        }

        return Optional.empty();
    }
}
