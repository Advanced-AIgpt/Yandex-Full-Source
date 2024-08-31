package ru.yandex.alice.paskill.dialogovo.domain.converter;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import org.springframework.web.util.HtmlUtils;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.layout.div.BackgroundType;
import ru.yandex.alice.kronstadt.core.layout.div.Color;
import ru.yandex.alice.kronstadt.core.layout.div.DivAction;
import ru.yandex.alice.kronstadt.core.layout.div.DivAlignment;
import ru.yandex.alice.kronstadt.core.layout.div.DivAlignmentVertical;
import ru.yandex.alice.kronstadt.core.layout.div.DivBackground;
import ru.yandex.alice.kronstadt.core.layout.div.DivBody;
import ru.yandex.alice.kronstadt.core.layout.div.DivState;
import ru.yandex.alice.kronstadt.core.layout.div.ImageElement;
import ru.yandex.alice.kronstadt.core.layout.div.Size;
import ru.yandex.alice.kronstadt.core.layout.div.TextStyle;
import ru.yandex.alice.kronstadt.core.layout.div.block.Block;
import ru.yandex.alice.kronstadt.core.layout.div.block.ContainerBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.FooterBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.GalleryBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.ImageBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.NumericDivSize;
import ru.yandex.alice.kronstadt.core.layout.div.block.Position;
import ru.yandex.alice.kronstadt.core.layout.div.block.SeparatorBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.UniversalBlock;
import ru.yandex.alice.paskill.dialogovo.domain.AvatarsNamespace;
import ru.yandex.alice.paskill.dialogovo.domain.Image;
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.BigImageCard;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.BigImageListCard;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Card;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.CardButton;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListCard;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListCardHeader;
import ru.yandex.alice.paskill.dialogovo.processor.DialogovoDirectiveFactory;

import static java.util.stream.Collectors.toList;
import static ru.yandex.alice.kronstadt.core.layout.div.block.PredefinedDivSize.MATCH_PARENT;
import static ru.yandex.alice.kronstadt.core.layout.div.block.PredefinedDivSize.WRAP_CONTENT;

@SuppressWarnings("DuplicatedCode")
@Component
public class CardConverter {

    private final DialogovoDirectiveFactory directiveFactory;
    private final String imageGalleryBaseUrl;
    private final String avatarsUrl;

    public CardConverter(
            DialogovoDirectiveFactory directiveFactory,
            @Value("${imageGalleryBaseUrl}") String imageGalleryBaseUrl,
            @Value("${avatarsUrl}") String avatarsUrl
    ) {

        this.directiveFactory = directiveFactory;
        this.imageGalleryBaseUrl = imageGalleryBaseUrl;
        this.avatarsUrl = avatarsUrl;
    }

    /**
     * Convert card to Div notation
     *
     * @param card
     * @param additionalButtons
     * @return Div card
     */
    public Optional<DivBody> convert(String skillId, Card card, List<Button> additionalButtons) {
        switch (card.getType()) {
            case BIG_IMAGE:
                return Optional.of(convert((BigImageCard) card, additionalButtons));
            case ITEMS_LIST:
                return Optional.of(convert((ItemsListCard) card, additionalButtons));
            case BIG_IMAGE_LIST:
            case IMAGE_GALLERY:
                return Optional.of(convert(skillId, (BigImageListCard) card, additionalButtons));
            case INVALID_CARD:
                return Optional.empty();
            default:
                throw new IllegalArgumentException("unknown card type: " + card.getType());
        }
    }

    private DivBody convert(BigImageCard source, List<Button> additionalButtons) {
        boolean additionalButtonsEmpty = additionalButtons.isEmpty();
        boolean titlePresent = source.getTitle().isPresent() || source.getDescription().isPresent();

        var blocks = new ArrayList<Block>();

        final AvatarsNamespace namespace = source.getMdsNamespace()
                .map(AvatarsNamespace::createCustom)
                .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD);

        var imageButtonAction = source.getButton()
                .map(x -> createButtonAction(x, "big_image_whole_card", source.getTitle()));
        var imageUrl = getImageUrl(
                source.getImageId(),
                namespace,
                ImageAlias.ONE_X3);

        if (additionalButtonsEmpty) {
            blocks.add(ImageBlock.create(imageUrl, 2.24));
        } else {
            blocks.add(ImageBlock.create(imageUrl, 2.24, imageButtonAction.orElse(null)));
        }


        if (titlePresent) {
            blocks.add(SeparatorBlock.withoutDelimiter(Size.XXS));
            var imageTitleButton = UniversalBlock.builder()
                    .title(source.getTitle().map(HtmlUtils::htmlEscape).orElse(null))
                    .text(source.getDescription()
                            .map(x -> Color.coloredText(x, Color.GRAY_DEFAULT))
                            .orElse(null)
                    )
                    .action(additionalButtonsEmpty ? null : imageButtonAction.orElse(null))
                    .build();
            blocks.add(imageTitleButton);
        }

        if (additionalButtonsEmpty) {
            if (titlePresent) {
                blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));
            }
            return createDefaultBody(blocks, imageButtonAction);
        } else {
            addButtonsToDivCard(additionalButtons, blocks);
            blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));
            return createDefaultBody(blocks);
        }
    }

    private DivBody convert(String skillId, BigImageListCard source, List<Button> additionalButtons) {

        var blocks = new ArrayList<Block>();
        Optional<ItemsListCardHeader> header = source.getHeader();

        //добавляем блок с хедером галлереи, если присутствует
        if (header.isPresent() &&
                header.get().getText().isPresent() &&
                StringUtils.isNoneBlank(header.get().getText().get())
        ) {
            blocks.add(UniversalBlock.builder()
                    .title(header.flatMap(ItemsListCardHeader::getText)
                            .map(HtmlUtils::htmlEscape)
                            .orElse(null)
                    )
                    .titleStyle(TextStyle.TITLE_S)
                    .build()
            );
        }
        blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));

        var galleryItems = new ArrayList<ContainerBlock>(source.getItems().size());
        var imageIds = source.getItems().stream().map(BigImageListCard.Item::getImageId).collect(toList());
        for (int i = 0; i < source.getItems().size(); i++) {
            BigImageListCard.Item item = source.getItems().get(i);

            final AvatarsNamespace namespace = item.getMdsNamespace()
                    .map(AvatarsNamespace::createCustom)
                    .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD);

            String logId = "image_gallery_item_" + i;
            String imageUrl = getImageUrl(item.getImageId(), namespace, ImageAlias.ORIG);
            var galleryImageUrl = createGalleryImageUrl(skillId, imageIds, i);

            var directive = directiveFactory.createLinkDirective(galleryImageUrl);
            var openWebGalleryAction = new DivAction(logId, imageUrl, directive);

            var itemSelectedActionO = item.getButton()
                    .map(cardBtn -> createButtonAction(cardBtn, logId, item.getTitle()))
                    .orElse(null);

            List<Block> galleryRow = new ArrayList<>();
            ImageBlock image = ImageBlock.create(imageUrl, 0.7d, itemSelectedActionO);
            galleryRow.add(ContainerBlock.vertical(MATCH_PARENT, WRAP_CONTENT)
                    .child(image)
                    .frame(ContainerBlock.Frame.onlyRoundCorners())
                    .alignmentHorizontal(DivAlignment.center)
                    .build());


            if (item.getTitle().isPresent() || item.getDescription().isPresent()) {
                galleryRow.add(SeparatorBlock.withoutDelimiter(Size.XXS));
                var imageTitleButton = UniversalBlock.builder()
                        .title(item.getTitle().map(HtmlUtils::htmlEscape).orElse(null))
                        .titleStyle(TextStyle.TITLE_S)
                        .titleMaxLines(2)
                        .text(item.getDescription()
                                .map(HtmlUtils::htmlEscape)
                                .map(x -> Color.coloredText(x, Color.GRAY_DEFAULT))
                                .orElse(null)
                        )
                        .textStyle(TextStyle.TEXT_S)
                        .textMaxLines(4)
                        .action(itemSelectedActionO)
                        .build();

                // wrap title+description into container
                galleryRow.add(
                        ContainerBlock.vertical(MATCH_PARENT, WRAP_CONTENT)
                                .child(imageTitleButton)
                                //.frame(ContainerBlock.Frame.border(0xBBBBBB))
                                .build()
                );
            }


            galleryRow.add(SeparatorBlock.withDelimiter(Size.XS));

            galleryRow.add(new FooterBlock(
                    Color.coloredText("ОТКРЫТЬ КАРТИНКУ", Color.BLUE_LINK), openWebGalleryAction
            ));

            galleryItems.add(ContainerBlock.vertical(NumericDivSize.dp(300), WRAP_CONTENT)
                    .children(galleryRow)
                    .frame(ContainerBlock.Frame.border(0x7f7f7f))
                    .alignmentHorizontal(DivAlignment.center)
                    .alignmentVertical(DivAlignmentVertical.top)
                    .build()
            );


        }
        var gallery = GalleryBlock.create(galleryItems);
        blocks.add(gallery);

        if (!additionalButtons.isEmpty()) {
            addButtonsToDivCard(additionalButtons, blocks);
        }

        blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));
        return createDefaultBody(blocks);
    }

    private String createGalleryImageUrl(String skillId, List<String> imageIds, int index) {
        String uri = UriComponentsBuilder.fromHttpUrl(imageGalleryBaseUrl)
                .queryParam("skill_id", skillId)
                .queryParam("image_ids", String.join(",", imageIds))
                .queryParam("initial_index", index)
                .build()
                .toUriString();
        return uri;

    }


    private DivBody convert(ItemsListCard source, List<Button> additionalButtons) {
        var blocks = new ArrayList<Block>();
        var header = source.getHeader();
        if (header.isPresent() && header.get().getText().isPresent()) {
            blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));
            blocks.add(UniversalBlock.withTitleOnly(
                    header.get().getText().map(HtmlUtils::htmlEscape),
                    TextStyle.TITLE_S));
        } else {
            blocks.add(SeparatorBlock.withoutDelimiter(Size.XXS));
        }

        for (var item : source.getItems()) {
            final AvatarsNamespace namespace = item.getMdsNamespace()
                    .map(AvatarsNamespace::createCustom)
                    .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD);
            var image = item.getImageId()
                    .map(x -> getImageUrl(x, namespace, ImageAlias.MENU_LIST_X3))
                    .map(x -> new UniversalBlock.SideElement(new ImageElement(x, 1), Size.M, Position.LEFT));

            var action = item.getButton().map(x -> createButtonAction(x, "list_item", item.getTitle()));

            var block = UniversalBlock.builder()
                    .title(item.getTitle().map(HtmlUtils::htmlEscape).orElse(null))
                    .text(item.getDescription().map(x -> Color.coloredText(x, Color.GRAY_DEFAULT)).orElse(null))
                    .sideElement(image.orElse(null))
                    .action(action.orElse(null))
                    .build();

            blocks.add(block);
        }

        if (!additionalButtons.isEmpty()) {
            addButtonsToDivCard(additionalButtons, blocks);
        }

        var footerO = source.getFooter();
        if (footerO.isPresent() && footerO.get().getText().isPresent()) {
            var footer = footerO.get();
            var action = footer.getButton().map(x -> createButtonAction(x, "external_card_footer",
                    Optional.empty()));

            blocks.add(SeparatorBlock.withDelimiter(Size.XS));
            blocks.add(new FooterBlock(
                    Color.coloredText(footer.getText().get().toUpperCase(), Color.BLUE_LINK), action.orElse(null)
            ));
        } else {
            blocks.add(SeparatorBlock.withoutDelimiter(Size.XXS));
        }


        return createDefaultBody(blocks);
    }

    private void addButtonsToDivCard(List<Button> additionalButtons, ArrayList<Block> blocks) {
        int i = 0;
        for (Button btn : additionalButtons) {
            blocks.add(SeparatorBlock.withDelimiter(Size.XS));
            blocks.add(convertAdditionalButton(btn, i++));
        }
    }

    public String getImageUrl(String imageId, AvatarsNamespace namespace, ImageAlias alias) {
        return Image.getImageUrl(imageId, namespace, alias, avatarsUrl);
    }

    private Block convertAdditionalButton(Button btn, int i) {

        DivAction action = createButtonAction(btn, "skill_response_button_" + i);
        String coloredTitle = Color.coloredText(HtmlUtils.htmlEscape(btn.getText()), Color.BLUE_BUTTON);

        return UniversalBlock.builder()
                .title(coloredTitle)
                .titleStyle(TextStyle.TEXT_L)
                .titleMaxLines(2)
                .action(action)
                .build();
    }

    private DivBody createDefaultBody(List<Block> blocks) {
        return DivBody.builder()
                .background(List.of(new DivBackground(Color.WHITE, BackgroundType.SOLID)))
                .states(List.of(new DivState(1, blocks, null)))
                .build();
    }

    private DivBody createDefaultBody(List<Block> blocks, Optional<DivAction> divAction) {
        return DivBody.builder()
                .background(List.of(new DivBackground(Color.WHITE, BackgroundType.SOLID)))
                .states(List.of(new DivState(1, blocks, divAction.orElse(null))))
                .build();
    }

    private DivAction createButtonAction(CardButton button, String logId, Optional<String> titleFallback) {
        Optional<String> text = button.getText()
                .or(() -> titleFallback).map(HtmlUtils::htmlEscape);
        var directives = directiveFactory.createButtonDirectives(
                text, button.getUrl(), Optional.ofNullable(button.getPayload())
        );

        return new DivAction(logId, button.getUrl().orElse(null), directives);
    }

    private DivAction createButtonAction(Button button, String logId) {
        var directives = directiveFactory.createButtonDirectives(
                Optional.of(button.getText()),
                Optional.ofNullable(button.getUrl()),
                Optional.ofNullable(button.getPayload())
        );

        return new DivAction(logId, /*"<STUB>"*/button.getUrl(), directives);
    }
}
