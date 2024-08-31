import { ContainerBlock, Div, FixedSize, MatchParentSize, TextBlock, WrapContentSize } from 'divcard2';
import {
    colorWhiteOpacity60,
    offsetFromEdgeOfScreen,
    simpleBackground,
    skillNameColorTitle,
} from '../../style/constants';
import { text28m, title44m } from '../../style/Text/Text';
import EmptyDiv from '../../components/EmptyDiv';
import { ICardInfo } from '../mainScreen2_0/Cards';

interface Props {
    widgets: ICardInfo[];
}

const COLUMN_COUNT = 3;

function WidgetWrapper(widget : ICardInfo, index : number) : Div {
    return new ContainerBlock({
        height: new FixedSize({ value: 345 }),
        content_alignment_horizontal: 'center',
        orientation: 'vertical',
        margins: {
            bottom: 36,
            left: index % COLUMN_COUNT !== 0 ? 24 : 0,
        },
        items: [
            widget.div,
            new ContainerBlock({
                width: new MatchParentSize(),
                margins: {
                    top: 20,
                },
                content_alignment_horizontal: 'center',
                items: [
                    new TextBlock({
                        ...text28m,
                        width: new MatchParentSize(),
                        height: new WrapContentSize(),
                        auto_ellipsize: 1,
                        text_alignment_horizontal: 'center',
                        text: widget.name,
                        text_color: colorWhiteOpacity60,
                    }),
                ],
            }),
        ],
    });
}

export default function WidgetsListWrapper({ widgets }: Props): Div {
    const rows: Div[][] = [];
    widgets.forEach((widget, index) => {
        if (index % COLUMN_COUNT === 0) {
            rows.push([WidgetWrapper(widget, index)]);
        } else {
            rows[rows.length - 1].push(WidgetWrapper(widget, index));
        }
    });

    while (rows[rows.length - 1].length % COLUMN_COUNT !== 0) {
        rows[rows.length - 1].push(new EmptyDiv({
            width: new MatchParentSize(),
        }));
    }

    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        paddings: {
            left: 48,
            right: 48,
            top: 15,
        },
        background: simpleBackground,
        items: [
            new TextBlock({
                ...title44m,
                text_color: skillNameColorTitle,
                text: 'Добавление виджета',
                paddings: {
                    top: offsetFromEdgeOfScreen,
                    bottom: offsetFromEdgeOfScreen,
                },
            }),
            new ContainerBlock({
                height: new MatchParentSize({}),
                orientation: 'vertical',
                items: rows.map((row : Div[]) => {
                    return new ContainerBlock({
                        height: new MatchParentSize(),
                        items: row,
                        orientation: 'horizontal',
                    });
                }),
            }),
        ],
    });
}
