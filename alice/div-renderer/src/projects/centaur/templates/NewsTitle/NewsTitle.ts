import {
    ContainerBlock,
    MatchParentSize,
    WrapContentSize,
    ImageBlock,
    FixedSize,
    TextBlock,
    SolidBackground,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { colorWhite, offsetFromEdgeOfPartOfScreen } from '../../style/constants';
import { title36m, title48m } from '../../style/Text/Text';
import { formDate, generateBackground } from '../news/common';
import { IColorSet } from '../../style/colorSet';

export type NewsTitleProps = {
    item: NAlice.NData.ITNewsItem;
    topic: string;
    tz: string;
    theme: IColorSet;
}

export function NewsTitle({ item, topic, tz, theme }: NewsTitleProps, withBackgroundImage = false) {
    const newsTitle = new ContainerBlock({
        paddings: {
            top: offsetFromEdgeOfPartOfScreen,
            left: offsetFromEdgeOfPartOfScreen,
            bottom: offsetFromEdgeOfPartOfScreen,
            right: offsetFromEdgeOfPartOfScreen,
        },
        content_alignment_horizontal: 'center',
        content_alignment_vertical: 'center',
        width: new MatchParentSize({ weight: 1 }),
        height: new MatchParentSize({ weight: 1 }),
        items: compact([
            new ContainerBlock({
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'center',
                width: new WrapContentSize(),
                orientation: 'horizontal',
                items: compact([
                    item.Logo && new ContainerBlock({
                        background: [
                            new SolidBackground({
                                color: colorWhite,
                            }),
                        ],
                        content_alignment_horizontal: 'center',
                        content_alignment_vertical: 'center',
                        border: {
                            corner_radius: 16,
                        },
                        width: new FixedSize({ value: 48 }),
                        height: new FixedSize({ value: 48 }),
                        margins: {
                            right: 16,
                        },
                        items: [
                            new ImageBlock({
                                width: new FixedSize({ value: 36 }),
                                height: new FixedSize({ value: 36 }),
                                image_url: item.Logo,
                            }),
                        ],
                    }),
                    item.Agency && new TextBlock({
                        ...title36m,
                        text: item.Agency,
                        text_color: theme.textColor,
                    }),
                ]),
            }),
            item.Text && new TextBlock({
                text_alignment_horizontal: 'center',
                ...title48m,
                text: item.Text,
                text_color: theme.textColor,
                max_lines: 4,
                margins: {
                    top: 32,
                },
            }),
            item.PubDate && new TextBlock({
                ...title36m,
                text_alignment_horizontal: 'center',
                text: (Number(item.PubDate) ? formDate(Number(item.PubDate) * 1000 || 0, tz) : ''),
                text_color: theme.textColor,
                alpha: 0.5,
                margins: {
                    top: 32,
                },
            }),
        ]),
    });

    if (withBackgroundImage) {
        newsTitle.background = generateBackground(topic, item.Image, theme.dimeColor);
    }
    return newsTitle;
}
