import {
    ContainerBlock,
    Div,
    FixedSize,
    ImageBlock,
    SolidBackground,
    Template,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { text28m, title32m } from '../../../../style/Text/Text';
import { colorWhiteOpacity10, colorWhiteOpacity50 } from '../../../../style/constants';
import { IRequestState } from '../../../../../../common/types/common';
import { createRequestState } from '../../../../../../registries/common';

export const MUSIC_CARD_COVER_SIZE = 275;

function MusicCover({
    imageUrl,
    imageBorderRadius,
}: {
    imageUrl: string | Template,
    imageBorderRadius: number | Template,
}) {
    return new ImageBlock({
        width: new FixedSize({ value: MUSIC_CARD_COVER_SIZE }),
        height: new FixedSize({ value: MUSIC_CARD_COVER_SIZE }),
        background: [
            new SolidBackground({ color: colorWhiteOpacity10 }),
        ],
        image_url: imageUrl ?? '',
        preload_required: 1,
        border: {
            corner_radius: imageBorderRadius,
        },
    });
}

export interface IMusicCardTemplateProps {
    cover_url: string;
    border_radius: number;
    title: string;
    subtitle?: string;
    item_index: number;
}

export function MusicCardTemplate(withSubtitle = false): [Div, IRequestState] {
    return [new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        orientation: 'vertical',
        items: compact([
            MusicCover({
                imageUrl: new Template('cover_url'),
                imageBorderRadius: new Template('border_radius'),
            }),
            new TextBlock({
                ...title32m,
                width: new FixedSize({ value: MUSIC_CARD_COVER_SIZE }),
                height: new WrapContentSize(),
                paddings: {
                    top: 20,
                },
                text: new Template('title'),
                max_lines: 2,
            }),
            withSubtitle &&
            new TextBlock({
                ...text28m,
                text_color: colorWhiteOpacity50,
                width: new FixedSize({ value: MUSIC_CARD_COVER_SIZE }),
                height: new WrapContentSize(),
                text: new Template('subtitle'),
                max_lines: 1,
            }),
        ]),
        extensions: [
            {
                id: 'centaur-analytics',
                params: {
                    id: 'main-screen-music-tab-gallery-element',
                    title: new Template('title'),
                    type: 'music',
                    pos: new Template('item_index'),
                },
            },
        ],
    }), createRequestState()];
}
