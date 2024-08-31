package ru.yandex.alice.paskills.my_alice.controller;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Controller;

import ru.yandex.alice.paskills.my_alice.apphost.request_init.RequestInit;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.Block;
import ru.yandex.alice.paskills.my_alice.blocks.BlockRender;
import ru.yandex.alice.paskills.my_alice.blocks.BlockType;
import ru.yandex.alice.paskills.my_alice.blocks.MultipleBlocksWithRequest;
import ru.yandex.alice.paskills.my_alice.blocks.SingleBlockRender;
import ru.yandex.alice.paskills.my_alice.blocks.computervision.ComputerVision;
import ru.yandex.alice.paskills.my_alice.blocks.music.Music;
import ru.yandex.alice.paskills.my_alice.blocks.recommender.RecommenderBlock;
import ru.yandex.alice.paskills.my_alice.blocks.skills.Skills;
import ru.yandex.alice.paskills.my_alice.blocks.station.StationPromo;
import ru.yandex.alice.paskills.my_alice.layout.ListGroup;
import ru.yandex.alice.paskills.my_alice.layout.PageLayout;
import ru.yandex.alice.paskills.my_alice.layout.PlusCard;
import ru.yandex.web.apphost.api.request.RequestContext;

@Controller
public class ProcessHandler implements PathHandler {

    private static final Logger logger = LogManager.getLogger();

    private final RequestInit requestInit;

    private final List<SingleBlockRender> singleBlockRenders;
    private final List<MultipleBlocksWithRequest> multipleBlocksWithRequests;

    ProcessHandler(
            RequestInit requestInit,
            Music music,
            StationPromo stationPromo,
            Skills skills,
            ComputerVision computerVision,
            RecommenderBlock recommenderBlock
    ) {
        this.requestInit = requestInit;
        this.singleBlockRenders = List.of(
                recommenderBlock,
                music,
                computerVision,
                stationPromo
        ).stream().filter(BlockRender::isEnabled).collect(Collectors.toUnmodifiableList());
        this.multipleBlocksWithRequests = List.of(
                skills
        ).stream().filter(BlockRender::isEnabled).collect(Collectors.toUnmodifiableList());
    }

    @Override
    public String getPath() {
        return "/process";
    }

    @Override
    public void handle(RequestContext ctx) {
        logger.debug("Process request context: {}", ctx);
        SessionId.Response sessionId = requestInit.getSessionId(ctx);

        Map<BlockType, PageLayout.Group> blocks = new HashMap<>();
        for (SingleBlockRender renderer : singleBlockRenders) {
            try {
                renderer.render(ctx, sessionId).ifPresent(b -> blocks.put(b.getType(), b.getGroup()));
            } catch (Exception e) {
                // TODO: add solomon sensor
                logger.error("Failed to render block {}", renderer, e);
            }
        }
        for (MultipleBlocksWithRequest renderer : multipleBlocksWithRequests) {
            try {
                List<Block> renderedBlocks = renderer.render(ctx, sessionId);
                for (var skillBlock : renderedBlocks) {
                    blocks.put(skillBlock.getType(), skillBlock.getGroup());
                }
            } catch (Exception e) {
                // TODO: add solomon sensor
                logger.error("Failed to render block {}", renderer, e);
            }
        }

        List<BlockType> orderedBlocks = sessionId.getUser() != null &&
                sessionId.getUser().getAttributes().isHavePlus() ?
                BlockType.ORDERED :
                BlockType.ORDERED_NO_PLUS;
        List<PageLayout.Group> groups = orderedBlocks.stream()
                .map(blocks::get)
                .filter(Objects::nonNull)
                .collect(Collectors.toList());
        var mainPage = new PageLayout(groups);
        ctx.addJsonItem("my-alice-main-page", mainPage);
    }

    private PageLayout.Group buildPlusPromoBlock() {
        return new ListGroup(
                null,
                List.of(
                        new PlusCard(
                                "Оформить подписку на Плюс",
                                "1 месяц бесплатно с подпиской Плюс",
                                "Затем 269 ₽ в месяц"
                        )
                ),
                null
        );
    }


}
