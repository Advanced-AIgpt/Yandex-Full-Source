package ru.yandex.alice.paskills.my_alice.blocks;

import java.util.List;

import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;

interface MultipleBlockRender extends BlockRender {

    List<Block> render(RequestContext context, SessionId.Response blackboxResponse);

}
