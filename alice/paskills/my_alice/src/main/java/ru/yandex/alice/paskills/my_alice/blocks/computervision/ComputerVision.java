package ru.yandex.alice.paskills.my_alice.blocks.computervision;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.Block;
import ru.yandex.alice.paskills.my_alice.blocks.BlockType;
import ru.yandex.alice.paskills.my_alice.blocks.SingleBlockRender;
import ru.yandex.alice.paskills.my_alice.layout.Card;
import ru.yandex.alice.paskills.my_alice.layout.ExpandableListGroup;
import ru.yandex.alice.paskills.my_alice.layout.PageLayout;
import ru.yandex.web.apphost.api.request.RequestContext;

@Component
public class ComputerVision implements SingleBlockRender {

    private static final List<PageLayout.Card> CARDS = List.of(
            new Card(
                    Card.Kind.SCENARIO,
                    "Распознать объект на фото",
                    "Справки даю!",
                    "Покажите мне собаку, а я назову породу",
                    "https://avatars.mds.yandex.net/get-dialogs/399212/deb2b751fc36c1efcb1e/orig",
                    "#ebfaef",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Найти товар по фото",
                    "Поиск по фото",
                    "Найду по фотографии и скажу, где купить",
                    "https://avatars.mds.yandex.net/get-dialogs/1676983/88668eb9c7128713dba2/orig",
                    "#ebfaef",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Прочти текст",
                    "Читаю с картинки",
                    "Прочту мелкий шрифт без лупы",
                    "https://avatars.mds.yandex.net/get-dialogs/1017510/167eeab1842965ba8849/orig",
                    "#ffe8e8",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Найти похожие картинки",
                    "Веселые картинки",
                    "Помогу найти изображения, которые вас вдохновляют",
                    "https://avatars.mds.yandex.net/get-dialogs/1017510/728808190058fcb7ac5d/orig",
                    "#ebfaef",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Я художник",
                    "Это искусство!",
                    "Загрузите фото, и я покажу, на какой шедевр похоже",
                    "https://avatars.mds.yandex.net/get-dialogs/1676983/a83d7aafc953e001fc4a/orig",
                    "#e8eeff",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "На кого я похож",
                    "На кого вы похожи?",
                    "Есть ли в мире кто-то, похожий на вас? Спросите меня",
                    "https://avatars.mds.yandex.net/get-dialogs/399212/5583bc5cf20cf0e06f8d/orig",
                    "#f0ebfa",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Распознай код",
                    "Прочту QR-код",
                    "Сфотографируйте QR-код, и я переведу его в слова",
                    "https://avatars.mds.yandex.net/get-dialogs/1027858/a711fa8e5a57f7e336e6/orig",
                    "#e8eeff",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Сделай скан",
                    "Тишина, я сканирую",
                    "Подготовлю четкую версию документа",
                    "https://avatars.mds.yandex.net/get-dialogs/1676983/8b515089dae337cd3a73/orig",
                    "#f0ebfa",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Переведи текст на фото",
                    "Как это по-русски?",
                    "Сфотографируйте текст, и я переведу на русский китайское меню",
                    "https://avatars.mds.yandex.net/get-dialogs/399212/44dd5be087d9e405f39f/orig",
                    "#ebfaef",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Распознать ссылку из текста",
                    "Скиньте ссылочку",
                    "Сфотографируйте текст с ссылкой, а я ее открою",
                    "https://avatars.mds.yandex.net/get-dialogs/1017510/24c698c5e21358a12af9/orig",
                    "#ebfaef",
                    null
            ),
            new Card(
                    Card.Kind.SCENARIO,
                    "Распознай текст",
                    "Читаю с листа",
                    "Покажите мне фото текста, и я превращу его в txt",
                    "https://avatars.mds.yandex.net/get-dialogs/1017510/786dca96c55c0a9d58de/orig",
                    "#e8eeff",
                    null
            )
    );

    @Override
    public Optional<Block> render(RequestContext context, SessionId.Response blackboxResponse) {
        ArrayList<PageLayout.Card> shuffledCards = new ArrayList<>(CARDS);
        Collections.shuffle(shuffledCards);
        PageLayout.Group group = ExpandableListGroup.create("Посмотрите, что видит Алиса", shuffledCards, "Покажи ещё");
        return Optional.of(new Block(BlockType.COMPUTER_VISION, group));
    }

    @Override
    public boolean isEnabled() {
        return true;
    }
}
