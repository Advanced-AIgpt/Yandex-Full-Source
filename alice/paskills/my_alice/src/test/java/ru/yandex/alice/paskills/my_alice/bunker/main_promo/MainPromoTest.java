package ru.yandex.alice.paskills.my_alice.bunker.main_promo;

import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Random;

import org.junit.jupiter.api.Test;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.lang.Nullable;

import ru.yandex.alice.paskills.my_alice.bunker.client.BunkerClient;
import ru.yandex.alice.paskills.my_alice.bunker.client.NodeContent;
import ru.yandex.alice.paskills.my_alice.bunker.client.NodeInfo;
import ru.yandex.alice.paskills.my_alice.bunker.image_storage.ImageStorage;
import ru.yandex.alice.paskills.my_alice.layout.Card;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class MainPromoTest {

    private static MainPromoImpl mainPromoForData(Data data) {
        return new MainPromoImpl(new BunkerClientMock(), "", new ImageStorageMock(), new Random(0L), data);
    }

    @Test
    void showProbability() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(0.5, List.of(
                new Data.Block(null, 1.0, "everybody", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                ))
        )));

        var card = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of()));
        var empty = Optional.empty();

        assertEquals(empty, mainPromo.getCard(true));
        assertEquals(card, mainPromo.getCard(true));
        assertEquals(empty, mainPromo.getCard(true));
        assertEquals(card, mainPromo.getCard(true));
        assertEquals(empty, mainPromo.getCard(true));
        assertEquals(empty, mainPromo.getCard(true));
        assertEquals(card, mainPromo.getCard(true));
        assertEquals(card, mainPromo.getCard(true));
        assertEquals(card, mainPromo.getCard(true));
        assertEquals(empty, mainPromo.getCard(true));
    }

    @Test
    void authFilter() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(1.0, List.of(
                new Data.Block("required", 1.0, "required", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                )),
                new Data.Block("deny", 1.0, "deny", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                )),
                new Data.Block("everybody", 1.0, "everybody", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                ))
        )));


        var authOnlyCard = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "required"
        )));
        var noAuthCard = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "deny"
        )));
        var anyAuthCard = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "everybody"
        )));

        assertEquals(authOnlyCard, mainPromo.getCard(true));
        assertEquals(anyAuthCard, mainPromo.getCard(true));
        assertEquals(anyAuthCard, mainPromo.getCard(true));
        assertEquals(authOnlyCard, mainPromo.getCard(true));
        assertEquals(authOnlyCard, mainPromo.getCard(true));
        assertEquals(authOnlyCard, mainPromo.getCard(true));

        assertEquals(anyAuthCard, mainPromo.getCard(false));
        assertEquals(anyAuthCard, mainPromo.getCard(false));
        assertEquals(anyAuthCard, mainPromo.getCard(false));
        assertEquals(noAuthCard, mainPromo.getCard(false));
        assertEquals(anyAuthCard, mainPromo.getCard(false));
        assertEquals(noAuthCard, mainPromo.getCard(false));
    }

    @Test
    void blockWeight() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(1.0, List.of(
                new Data.Block("0", 0.0, "everybody", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                )),
                new Data.Block("1", 1.0, "everybody", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                )),
                new Data.Block("10", 10.0, "everybody", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                ))
        )));


        var block1 = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "1"
        )));
        var block10 = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "10"
        )));

        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block1, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block10, mainPromo.getCard(true));
        assertEquals(block1, mainPromo.getCard(true));
    }

    @Test
    void cardWeight() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(1.0, List.of(
                new Data.Block("block", 1.0, "everybody", List.of(
                        new Data.Card("0", 0.0, null, null, null, null, null),
                        new Data.Card("1", 1.0, null, null, null, null, null),
                        new Data.Card("10", 10.0, null, null, null, null, null)
                ))
        )));

        var card0 = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "block",
                "mainPromo_cardId", "0"
        )));
        var card1 = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "block",
                "mainPromo_cardId", "1"
        )));
        var card10 = Optional.of(new Card(Card.Kind.SCENARIO, null, null, null, null, null, Map.of(
                "mainPromo_blockId", "block",
                "mainPromo_cardId", "10"
        )));

        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card1, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card1, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
        assertEquals(card10, mainPromo.getCard(true));
    }

    @Test
    void blockZeroWeight() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(1.0, List.of(
                new Data.Block("block", 0.0, "everybody", List.of(
                        new Data.Card("0", 1.0, null, null, null, null, null)
                ))
        )));

        for (int i = 10000; i > 0; --i) {
            assertEquals(Optional.empty(), mainPromo.getCard(true));
        }
    }

    @Test
    void cardZeroWeight() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(1.0, List.of(
                new Data.Block("block", 1.0, "everybody", List.of(
                        new Data.Card("0", 0.0, null, null, null, null, null)
                ))
        )));

        for (int i = 10000; i > 0; --i) {
            assertEquals(Optional.empty(), mainPromo.getCard(true));
        }
    }

    @Test
    void cardContent() {
        MainPromoImpl mainPromo = mainPromoForData(new Data(1.0, List.of(
                new Data.Block("block", 1.0, "everybody", List.of(
                        new Data.Card("card", 1.0, "CAPTION", "TEXT", "BUTTON", "IMG", "#fc0")
                ))
        )));

        var card = Optional.of(new Card(
                Card.Kind.SCENARIO,
                "BUTTON",
                "CAPTION",
                "TEXT",
                "uri was: IMG",
                "#fc0",
                Map.of(
                        "mainPromo_blockId", "block",
                        "mainPromo_cardId", "card"
                )
        ));

        assertEquals(card, mainPromo.getCard(true));
    }

    protected static class BunkerClientMock implements BunkerClient {
        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, Class<T> responseType) {
            return Optional.empty();
        }

        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, ParameterizedTypeReference<T> responseType) {
            return Optional.empty();
        }

        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, String version, Class<T> responseType) {
            return Optional.empty();
        }

        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, String version,
                                                ParameterizedTypeReference<T> responseType) {
            return Optional.empty();
        }

        @Override
        public Map<String, NodeInfo> tree(String rootNodePath) {
            return Map.of();
        }

        @Override
        public Map<String, NodeInfo> tree(String rootNodePath, String version) {
            return Map.of();
        }
    }

    private static class ImageStorageMock implements ImageStorage {
        @Override
        public Optional<String> getRealUrl(@Nullable String url) {
            return Optional.ofNullable(url).map(s -> "uri was: " + s);
        }

        @Override
        public boolean isReady() {
            return true;
        }
    }
}
