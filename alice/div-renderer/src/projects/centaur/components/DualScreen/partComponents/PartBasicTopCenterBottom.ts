import {
    ContainerBlock,
    Div, DivSize,
    MatchParentSize,
} from 'divcard2';
import { offsetFromEdgeOfPartOfScreen, offsetFromEdgeOfScreen } from '../../../style/constants';
import EmptyDiv from '../../EmptyDiv';

type ContainerDivOptions = Omit<Partial<ConstructorParameters<typeof ContainerBlock>[0]>, 'items'>;

interface IPartBasicTopCenterBottomProps {
    topDivItems?: Div[];
    topDivOptions?: ContainerDivOptions;
    middleDivItems?: Div[];
    middleDivOptions?: ContainerDivOptions;
    bottomDivItems?: Div[];
    bottomDivOptions?: ContainerDivOptions;
    containerOptions?: ContainerDivOptions;
    topSize?: DivSize;
    bottomSize?: DivSize;
}

export default function PartBasicTopCenterBottom({
    topDivItems,
    topDivOptions,
    topSize,
    middleDivItems,
    middleDivOptions,
    bottomDivItems,
    bottomDivOptions,
    bottomSize,
    containerOptions,
}: IPartBasicTopCenterBottomProps): Div[] {
    return [new ContainerBlock({
        height: new MatchParentSize({ weight: 1 }),
        width: new MatchParentSize({ weight: 1 }),
        paddings: {
            top: offsetFromEdgeOfPartOfScreen,
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfPartOfScreen,
        },
        ...containerOptions,
        items: [
            new ContainerBlock({
                orientation: 'horizontal',
                content_alignment_vertical: 'center',
                width: new MatchParentSize({ weight: 1 }),
                height: topSize,
                ...topDivOptions,
                items: topDivItems || [new EmptyDiv()],
            }),
            new ContainerBlock({
                height: new MatchParentSize({ weight: 1 }),
                margins: {
                    right: offsetFromEdgeOfScreen - offsetFromEdgeOfPartOfScreen,
                },
                content_alignment_vertical: 'center',
                ...middleDivOptions,
                items: middleDivItems || [new EmptyDiv()],
            }),
            new ContainerBlock({
                height: bottomSize,
                margins: {
                    right: offsetFromEdgeOfScreen - offsetFromEdgeOfPartOfScreen,
                },
                content_alignment_vertical: 'bottom',
                ...bottomDivOptions,
                items: bottomDivItems || [new EmptyDiv()],
            }),
        ],
    })];
}
