import {
    ContainerBlock, Div,
    FixedSize, ImageBackground,
    ImageBlock,
    MatchParentSize,
    SolidBackground,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { colorWhite, offsetFromEdgeOfPartOfScreen } from '../../../style/constants';
import { title36m, title48m } from '../../../style/Text/Text';
import { IColor, IColorSet } from '../../../style/colorSet';

interface IPartSimpleTextProps {
    source?: {
        logo?: string;
        name?: string;
    };
    text?: string;
    subText?: string;
    colorSet: IColorSet;
    backgroundImage?: string;
    backgroundColor?: IColor;
}

/**
 * @param source
 * @param text
 * @param subText
 * @param colorSet
 * @param backgroundImage
 * @example ./PartImage.ts:1
 * @constructor
 */
export default function PartSimpleText({
    source,
    text,
    subText,
    colorSet,
    backgroundImage,
    backgroundColor,
}: IPartSimpleTextProps): Div[] {
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
            source && new ContainerBlock({
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'center',
                width: new WrapContentSize(),
                orientation: 'horizontal',
                items: compact([
                    source.logo && new ContainerBlock({
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
                        width: new FixedSize({ value: 44 }),
                        height: new FixedSize({ value: 44 }),
                        margins: {
                            right: 16,
                        },
                        items: [
                            new ImageBlock({
                                width: new FixedSize({ value: 32 }),
                                height: new FixedSize({ value: 32 }),
                                border: {
                                    corner_radius: 12,
                                },
                                image_url: source.logo,
                            }),
                        ],
                    }),
                    source.name && new TextBlock({
                        ...title36m,
                        text: source.name,
                        text_color: colorSet.textColor,
                    }),
                ]),
            }),
            text && new TextBlock({
                text_alignment_horizontal: 'center',
                ...title48m,
                text,
                text_color: colorSet.textColor,
                max_lines: 4,
                margins: {
                    top: 32,
                },
            }),
            subText && new TextBlock({
                ...title36m,
                text_alignment_horizontal: 'center',
                text: subText,
                text_color: colorSet.textColor,
                alpha: 0.5,
                margins: {
                    top: 32,
                },
            }),
        ]),
    });

    if (backgroundImage) {
        newsTitle.background = compact([
            new ImageBackground({
                image_url: backgroundImage,
                scale: 'fill',
                preload_required: 1,
            }),
            backgroundColor && new SolidBackground({ color: backgroundColor }),
        ]);
    }
    return [newsTitle];
}
