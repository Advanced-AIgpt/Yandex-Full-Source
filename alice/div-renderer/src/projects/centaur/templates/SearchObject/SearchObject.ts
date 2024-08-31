import {
    ContainerBlock,
    FixedSize,
    GalleryBlock,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';

import { NAlice } from '../../../../protos';
import { SuggestsBlock } from '../suggests';
import {
    colorLightGrey,
    colorWhiteOpacity90,
    offsetFromEdgeOfScreen,
    simpleBackground,
} from '../../style/constants';
import { sourceHost } from '../../components/sourceHost';
import { text40r, title72m } from '../../style/Text/Text';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';

const firstLineHeight = title72m.line_height;

const title = (t: string) =>
    new TextBlock({
        ...title72m,
        text: t,
        width: new WrapContentSize(),
        height: new WrapContentSize(),
    });

const text = (t: string) =>
    new TextBlock({
        ...text40r,
        text_color: colorWhiteOpacity90,
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        text: t,
    });

const avatar = (t: string) =>
    new ImageBlock({
        width: new FixedSize({ value: firstLineHeight }),
        height: new FixedSize({ value: firstLineHeight }),
        margins: {
            right: 32,
        },
        image_url: t,
        border: {
            corner_radius: 150,
        },
    });

const makeSearchUrl = (thumbnail: NAlice.NData.TSearchObjectData.ITGalleryImage): string =>
    (thumbnail.ThmbHref?.startsWith('/') ? 'https:' : '') +
    `${thumbnail.ThmbHref}&n=13`;

const gallery = (images: NAlice.NData.TSearchObjectData.ITGalleryImage[]) =>
    new GalleryBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        item_spacing: 28,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: images.map(
            image =>
                new ImageBlock({
                    height: new FixedSize({ value: 366 }),
                    width: new FixedSize({ value: 366 }),
                    border: { corner_radius: 20 },
                    placeholder_color: colorLightGrey,
                    image_url: makeSearchUrl(image),
                }),
        ),
    });

function capitalizeFirstLetter(string: string): string {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

export default function SearchObject(
    { Text, Title, Hostname, Image, GalleryImages }: NAlice.NData.ITSearchObjectData,
    mmRequest: MMRequest,
) {
    return new TemplateCard(new Templates({}), {
        log_id: 'search_object',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: new GalleryBlock({
                        width: new WrapContentSize(),
                        height: new MatchParentSize(),
                        background: simpleBackground,
                        paddings: {
                            top: 176,
                            bottom: offsetFromEdgeOfScreen,
                        },
                        item_spacing: 0,
                        orientation: 'vertical',
                        items: compact([
                            (Image || Title) &&
                            new ContainerBlock({
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                content_alignment_vertical: 'top',
                                margins: {
                                    bottom: 32,
                                    left: offsetFromEdgeOfScreen,
                                    right: offsetFromEdgeOfScreen,
                                },
                                orientation: 'horizontal',
                                items: compact([Image && avatar(Image), Title && title(Title)]),
                            }),
                            Text &&
                            new ContainerBlock({
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                orientation: 'horizontal',
                                margins: {
                                    bottom: 16,
                                    left: offsetFromEdgeOfScreen,
                                    right: offsetFromEdgeOfScreen,
                                },
                                items: [text(capitalizeFirstLetter(Text))],
                            }),
                            Hostname && new ContainerBlock({
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                margins: {
                                    bottom: 40,
                                    left: offsetFromEdgeOfScreen,
                                    right: offsetFromEdgeOfScreen,
                                },
                                orientation: 'horizontal',
                                items: [sourceHost(Hostname)],
                            }),
                            GalleryImages && gallery(GalleryImages),
                            SuggestsBlock(mmRequest.ScenarioResponseBody),
                        ]),
                    }),
                }),
            },
        ],
    });
}
