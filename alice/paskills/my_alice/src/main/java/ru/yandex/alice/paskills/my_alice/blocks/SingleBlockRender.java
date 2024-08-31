package ru.yandex.alice.paskills.my_alice.blocks;

import java.util.Optional;

import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;

public interface SingleBlockRender extends BlockRender {

    Optional<Block> render(RequestContext context, SessionId.Response blackboxResponse);

}
