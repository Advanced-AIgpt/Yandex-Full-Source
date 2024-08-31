import {
    ContainerBlock, Div,
    ImageBackground,
    MatchParentSize,
    SolidBackground,
    TextBlock,
} from 'divcard2';
import { compact } from 'lodash';
import { offsetFromEdgeOfPartOfScreen } from '../../../style/constants';
import {
    title130m,
    title36m,
    title48m,
    title56m,
    title64m,
} from '../../../style/Text/Text';
import { IColor, IColorSet } from '../../../style/colorSet';
import { textBreakpoint } from '../../../helpers/helpers';

interface IPartBigTextProps {
    text?: string;
    subText?: string;
    colorSet: IColorSet;
    backgroundImage?: string;
    backgroundColor?: IColor;
}

const wrapText = textBreakpoint(
    [20, title130m],
    [40, title64m],
    [80, title56m],
    [Number.POSITIVE_INFINITY, title48m],
);

/**
 * @param source
 * @param text
 * @param subText
 * @param colorSet
 * @param backgroundImage
 * @param backgroundColor
 * @constructor
 */
export default function PartBigText({
    text,
    subText,
    colorSet,
    backgroundImage,
    backgroundColor,
}: IPartBigTextProps): Div[] {
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
            text && new TextBlock({
                text_alignment_horizontal: 'center',
                ...wrapText(text),
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
                text_color: colorSet.textColorOpacity50,
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
