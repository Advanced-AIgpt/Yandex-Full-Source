package ru.yandex.alice.paskills.my_alice.blocks.music;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

@Data
public class MusicLandingResponse {

    private static final Logger logger = LogManager.getLogger();
    private static final int MAX_CARD_COUNT = 60;

    private final InvocationInfo invocationInfo;
    private final Result result;

    @Data
    private static class InvocationInfo {
        private final String hostname;
        private final String action;
        @JsonProperty("app-name")
        private final String appName;
        @JsonProperty("app-version")
        private final String appVersion;
        @JsonProperty("req-id")
        private final String reqId;
        @JsonProperty("exec-duration-millis")
        private final String execDurationMillis;
    }

    public List<BlockEntity> pickNext(Set<String> idBlacklist) {
        long validItemCounts = result.blocks
                .stream()
                .flatMap(b -> b.getEntities().stream())
                .filter(BlockEntity::isValid)
                .count();
        int maxItems = Math.min(
                ((int) Math.floor(validItemCounts / 6.)) * 6,
                MAX_CARD_COUNT
        );
        List<BlockEntity> nextItems = new ArrayList<>(maxItems);
        for (var entity: asIterable(idBlacklist)) {
            if (nextItems.size() < maxItems) {
                nextItems.add(entity);
            } else {
                break;
            }
        }
        return nextItems;
    }

    private Iterable<BlockEntity> asIterable(Set<String> idBlacklist) {
        return new BlockEntityIterable(idBlacklist);
    }

    private class BlockEntityIterator implements Iterator<BlockEntity> {

        private final Set<String> blacklist;

        private int blockId;
        private Map<Integer, Integer> blockPositions;

        private BlockEntityIterator(Set<String> blacklist) {
            this.blacklist = blacklist;
            this.blockId = 0;
            this.blockPositions = IntStream.range(0, result.blocks.size())
                    .boxed()
                    .collect(Collectors.toMap(Function.identity(), v -> 0));
        }

        @Override
        public boolean hasNext() {
            if (result.blocks.isEmpty()) {
                return false;
            }
            return peek().isPresent();
        }

        @Override
        public BlockEntity next() {
            var peek = peek();
            if (peek.isPresent()) {
                blockId = nextBlockId(peek.get().getBlockId());
                blockPositions.put(peek.get().getBlockId(), peek.get().getEntityId() + 1);
                return peek.get().getEntity();
            } else {
                throw new NoSuchElementException();
            }
        }

        private Optional<Peek> peek() {
            Set<Integer> visitedBlocks = new HashSet<>(result.getBlocks().size());
            for (int currentBlockID = this.blockId, i = 0;
                 !visitedBlocks.contains(currentBlockID) && i < 100;
                 currentBlockID = nextBlockId(currentBlockID), i++
            ) {
                MusicResponseBlock block = result.getBlocks().get(currentBlockID);
                for (
                        int entityId = blockPositions.get(currentBlockID);
                        entityId < result.getBlocks().get(currentBlockID).getEntities().size();
                        entityId++
                ) {
                    var entity = block.getEntities().get(entityId);
                    if (entity.isValid() && !blacklist.contains(entity.getId())) {
                        return Optional.of(new Peek(currentBlockID, entityId, entity));
                    } else if (!entity.isValid()) {
                        // TODO: increment solomon counter
                        logger.debug("Found invalid entity: {}", entity);
                    }
                }
                visitedBlocks.add(currentBlockID);
            }
            return Optional.empty();
        }

        private int nextBlockId(int currentBlockId) {
            return (currentBlockId + 1) % result.blocks.size();
        }

        @Data
        private class Peek {
            private final int blockId;
            private final int entityId;
            private final BlockEntity entity;
        }

    }

    private class BlockEntityIterable implements Iterable<BlockEntity> {

        private final Set<String> blacklist;

        private BlockEntityIterable(Set<String> blacklist) {
            this.blacklist = blacklist;
        }

        @Override
        public Iterator<BlockEntity> iterator() {
            return new BlockEntityIterator(blacklist);
        }
    }

}
