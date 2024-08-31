package ru.yandex.alice.paskills.my_alice.controller;

import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import NAppHostHttp.Http;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Controller;

import ru.yandex.alice.paskills.my_alice.apphost.blackbox_http.BlackboxHttp;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.BlockRender;
import ru.yandex.alice.paskills.my_alice.blocks.WithHttpRequest;
import ru.yandex.alice.paskills.my_alice.blocks.music.Music;
import ru.yandex.alice.paskills.my_alice.blocks.recommender.RecommenderBlock;
import ru.yandex.alice.paskills.my_alice.blocks.skills.Skills;
import ru.yandex.alice.paskills.my_alice.blocks.station.StationPromo;
import ru.yandex.web.apphost.api.request.RequestContext;

@Controller
public class PrepareHandler implements PathHandler {

    private static final Logger logger = LogManager.getLogger();

    private final BlackboxHttp blackboxHttp;

    private final List<WithHttpRequest> enabledBlocks;

    PrepareHandler(
            BlackboxHttp blackboxHttp,
            RecommenderBlock recommenderBlock,
            Music music,
            StationPromo stationPromo,
            Skills skills
    ) {
        this.blackboxHttp = blackboxHttp;
        this.enabledBlocks = List.of(
                music,
                stationPromo,
                skills,
                recommenderBlock
        ).stream().filter(BlockRender::isEnabled).collect(Collectors.toUnmodifiableList());
    }

    @Override
    public String getPath() {
        return "/prepare";
    }

    @Override
    public void handle(RequestContext ctx) {
        // Parse blackbox response
        SessionId.Response blackBoxResponse = blackboxHttp.getSessionIdResponse(ctx);
        logger.debug("blackbox response: {}", blackBoxResponse);
        if (blackBoxResponse.getRawData() != null) {
            ctx.addJsonItem("blackbox", blackBoxResponse.getRawData());
        }
        for (WithHttpRequest source: enabledBlocks) {
            try {
                Optional<Http.THttpRequest> request = source.prepare(ctx, blackBoxResponse);
                request.ifPresentOrElse(
                        (req) -> {
                            logger.debug("adding {} to context: {}", source.getRequestContextKey(), req);
                            ctx.addProtobufItem(source.getRequestContextKey(), req);
                        },
                        () -> {
                            logger.warn("{} is empty", source.getRequestContextKey());
                        }
                );
            } catch (Exception e) {
                // TODO: add solomon sensor
                logger.error("Failed to add {}", source.getRequestContextKey(), e);
            }
        }
    }


}
