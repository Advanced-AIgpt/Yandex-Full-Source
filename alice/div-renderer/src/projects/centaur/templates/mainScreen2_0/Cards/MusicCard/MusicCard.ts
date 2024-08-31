import {
    ContainerBlock,
    Div,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    SolidBackground,
    TextBlock,
} from 'divcard2';
import { compact } from 'lodash';
import { BasicCard } from '../BasicCard';
import { text28m, title32m, title48m } from '../../../../style/Text/Text';
import { colorWhiteOpacity10, colorWhiteOpacity50 } from '../../../../style/constants';
import EmptyDiv from '../../../../components/EmptyDiv';
import { IMusicCardProps } from './types';
import { logger } from '../../../../../../common/logger';
import { updatedAt } from '../../../../helpers/helpers';
import { getS3Asset } from '../../../../helpers/assets';
import { musicLoaderElement } from '../../../Music/MusicLoader/MusicLoader';
import { getCardMainScreenId } from '../helpers';

function MusicErrorCard({ rowIndex, colIndex }: {rowIndex?: number; colIndex?: number}): Div {
    return BasicCard({
        id: getCardMainScreenId({ rowIndex, colIndex }),
        items: [
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity50,
                text: 'Музыка',
            }),
            new EmptyDiv({
                height: new MatchParentSize({ weight: 2 }),
            }),
            new ContainerBlock({
                content_alignment_horizontal: 'center',
                items: [
                    new TextBlock({
                        ...title48m,
                        width: new FixedSize({ value: 135 }),
                        height: new FixedSize({ value: 135 }),
                        text_alignment_horizontal: 'center',
                        text_alignment_vertical: 'center',
                        line_height: 0,
                        text: '✕',
                        background: [
                            new SolidBackground({ color: colorWhiteOpacity10 }),
                        ],
                        border: {
                            corner_radius: 67,
                        },
                    }),
                    new TextBlock({
                        ...title32m,
                        margins: {
                            top: 24,
                        },
                        text_color: colorWhiteOpacity50,
                        text_alignment_horizontal: 'center',
                        text: 'Что-то пошло не так',
                    }),
                    new TextBlock({
                        ...text28m,
                        margins: {
                            top: 8,
                        },
                        text_color: colorWhiteOpacity50,
                        text_alignment_horizontal: 'center',
                        text: 'Проверьте интернет или обновите экран',
                    }),
                ],
            }),
            new EmptyDiv({
                height: new MatchParentSize({ weight: 3 }),
            }),
        ],
    });
}

export default function MusicCard({
    color,
    name,
    description,
    cover,
    modified,
    actions,
    longtap_actions,
    requestState,
    rowIndex,
    colIndex,
}: Partial<IMusicCardProps>): Div {
    if (!name || !color || (!description && !modified) || !cover) {
        logger.error('The music card should have a name, color, description and cover, but some of this was not transmitted');

        return MusicErrorCard({
            rowIndex,
            colIndex,
        });
    }

    const cardData: Parameters<typeof BasicCard>[0] = {
        id: getCardMainScreenId({ rowIndex, colIndex }),
        background: [new SolidBackground({ color })],
        items: [
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity50,
                text: 'Рекомендуем',
            }),
            new EmptyDiv({
                height: new MatchParentSize(),
            }),
            new ImageBlock({
                image_url: cover,
                height: new FixedSize({ value: 220 }),
                width: new FixedSize({ value: 220 }),
                alignment_horizontal: 'center',
                border: {
                    corner_radius: 18,
                },
            }),
            new TextBlock({
                ...title32m,
                text_alignment_horizontal: 'center',
                margins: {
                    top: 24,
                },
                text: name,
            }),
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity50,
                text_alignment_horizontal: 'center',
                margins: {
                    top: 4,
                },
                text: description || updatedAt(new Date(modified ?? 0)),
            }),
            new EmptyDiv({
                height: new MatchParentSize(),
            }),
            new ImageBlock({
                image_url: getS3Asset('player/play.png?v=1'),
                height: new FixedSize({ value: 79 }),
                width: new FixedSize({ value: 79 }),
                margins: {
                    bottom: 8,
                },
                alignment_horizontal: 'center',
            }),
        ],
        actions: actions ? compact([
            requestState && musicLoaderElement({
                title: name,
                subtitle: description,
                ImageUrl: cover,
            }, requestState),
            ...actions,
        ]) : undefined,
        longtap_actions,
    };

    return BasicCard(cardData);
}
